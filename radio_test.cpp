
#include <memory>
#include <thread>
#include <chrono>
#include <iostream>
#include <fstream> 
#include <complex>

#include "phy/RadioThread.h"
#include "phy/LimeRadioThread.h"
#include "lime/LimeSuite.h"
#include "liquid/liquid.h"
#include "util/log.h"


#include <nlohmann/json.hpp>
#include <cxxopts.hpp>

using json = nlohmann::json;

// #####################################################
// in ../build => g++ ../*.cpp ../phy/*.cpp -I.. -I../phy -I../external/spdlog/include -std=c++17 -pthread -lLimeSuite -L/usr/lib -o radio_test
// use cmake now!!!

// test program reading 2 sec of IQ data using a thread 
// RadioThread run() is reading the 2 sec data

// program is currently writing data into CSV file for futher
// analysis with octave



void write_csv_file(RadioThread* sdr, RadioThreadIQDataQueuePtr iqpipe_rx, std::string csv_file) {
        
    std::ofstream myfile(csv_file);

    // error checking if file is there ... maybe a bit overkill for a test script :-)
    if(!myfile.is_open()) {
        LOG_TEST_ERROR("cannot open {}", csv_file);

        LOG_TEST_ERROR("terminate thread and flush data");
        sdr->terminate();
        while(!sdr->isTerminated(100)) {
            //flush the iq queue so that it does not overrun
            LOG_TEST_ERROR("wait to terminate");
        }
        LOG_TEST_ERROR("thread is terminated");
    } else {

        // get the received IQ data via the queue which is attached to the SDR thread

        RadioThreadIQDataPtr iq;
        
        LOG_TEST_INFO("get data from queue and write to /tmp/ramdisk/IQData.csv ");
        while(sdr->isRxTxRunning()) {

            // get data from queue
            while(iqpipe_rx->pop(iq)) {

                uint64_t t = iq->timestampFirstSample;

                for(auto s: iq->data)
                {
                    if(myfile.is_open())
                        myfile << s.real() << ", " << s.imag()<< ", " << t << std::endl;
                    t++;
                }

            }

        }
        myfile.close();
        LOG_TEST_INFO("done write to /tmp/ramdisk/IQData.csv ");
    }

}


void warta(RadioThread *r, int64_t t) {

    // start to wait when RadioThread is collecting data
    while(!r->isRxTxRunning()) {
        std::cout << "x";
    }

    LOG_TEST_INFO("warta() for {} sec", t);
    std::this_thread::sleep_for(std::chrono::seconds(t));

    LOG_TEST_INFO("warta() done sleeping");
    r->stop();
}



int main(int argc, const char* argv[])
{

    // some generic system init must be implement which is setting up all the "utilities" stuff
    // init logging for spdlog
    Log::Init();

    // init cmdline parsing w/ cxxopts
    cxxopts::Options options("radio_test", "Test PHY SDR RX / TX");

    options
      .set_width(70)
      .add_options()
      ("h,help", "Print help")
      ("c,config", "Config file")
      ("w", "write CSV file")
      ("l", "gpio test")
    ;

    auto result = options.parse(argc,argv);

    if(result.count("help")) {
        std::cout << options.help({""}) << std::endl;
        exit(0);
    }




    LOG_TEST_INFO("SDR Radio test program");

    // hack - read config file --> must go into a system config class
    std::ifstream SystemConfigFile("SystemConfig.json");
    json SystemConfig = json::parse(SystemConfigFile);
    SystemConfigFile.close();

    float_t cf_samp_rate = SystemConfig["Radio"]["SAMPLING_RATE"];
    size_t cf_oversampling = SystemConfig["Radio"]["OVERSAMPLING"];
    float_t cf_center_freq = SystemConfig["Radio"]["CENTER_FREQ"];

    LOG_TEST_DEBUG("SystemConfig samprate {}", cf_samp_rate);
    LOG_TEST_DEBUG("SystemConfig oversamp {}", cf_oversampling);
    LOG_TEST_DEBUG("SystemConfig freq {}", cf_center_freq);


    // create SDR object
    RadioThread *sdr;

    // create RX and TX queues to communicate with the SDR object
    RadioThreadIQDataQueuePtr iqpipe_rx = std::make_shared<RadioThreadIQDataQueue>();
    iqpipe_rx->set_max_items(1000);

    RadioThreadIQDataQueuePtr iqpipe_tx = std::make_shared<RadioThreadIQDataQueue>();
    iqpipe_tx->set_max_items(1000);

    // init SDR
//    sdr = new LimeRadioThread(4500);  // set to a specifc sampleCnt (# samples RX , # samples TX max)
    sdr = new LimeRadioThread();  // default is defined via DEFAULT_SAMPLEBUFFERCNT
    sdr->setRXQueue(iqpipe_rx);
    sdr->setTXQueue(iqpipe_tx);
    sdr->setFrequency(cf_center_freq);
    sdr->setSamplingRate(cf_samp_rate, cf_oversampling);


    // create SDR Thread
    std::thread *t_sdr = nullptr;

    //Start RadioThread this is also starting the RX and TX streams
    t_sdr = new std::thread(&RadioThread::threadMain, sdr);

    //wait for waitTime seconds and then stop the sdr thread to limit the amount
    //of data collected for testing
    uint64_t waitTime = 2;
    std::thread w1(warta,sdr, waitTime);
    
 
    // wait until the thread started the run() which is handling the RX/TX comm to the sdr
    LOG_TEST_INFO("wait for receiver");
    while(!sdr->isRxTxRunning()) {
        std::cout << "x";
    }
    std::cout << std::endl;


    // as soon as receiver is running and is getting data it can be taken from the iqpiperx queue by some other process
    // process must be able to cope up with data so that queue is not overrunning .. otherwise samples are lost .. 
    // for the phy a 802.22 frame is approx 23k samples for our setup 

    // schleep a bit ..
    for(int i=0; i<100; i++)
        std::cout << ".";
    std::cout << std::endl;


    // write data for debug to ramdisk (cmdline -w option) - anyways  is even with ram very slow ...
    // root@x:~# mkdir /tmp/ramdisk
    // root@x:~# chmod 777 ramdisk/
    // root@x:~# mount -t tmpfs -o size=200M myramdisk /tmp/ramdisk/
    if(result.count("w")) {
        write_csv_file(sdr, iqpipe_rx,"/tmp/ramdisk/IQData.csv");
    } else {
        sdr->stop();
    }

    // test GPIOS w/ connected LED's /w cmdline -l option
    if(result.count("l")) {

        sdr->set_HW_RX();
        sleep(1);
        sdr->set_HW_TX(RadioThread::TxMode::TX_DIRECT);
        sleep(1);
        sdr->set_HW_TX(RadioThread::TxMode::TX_6M);
        sleep(1);
        sdr->set_HW_TX(RadioThread::TxMode::TX_2M);
        sleep(1);
        sdr->set_HW_TX(RadioThread::TxMode::TX_70CM);
        sleep(1);
        sdr->set_HW_RX();
        sleep(1);
        sdr->set_HW_TX();
        sleep(1);
        sdr->set_HW_TX_mode(RadioThread::TxMode::TX_6M);
        sdr->set_HW_TX();
        sleep(1);
        sdr->set_HW_TX_mode(RadioThread::TxMode::TX_2M);
        sdr->set_HW_TX();
        sleep(1);
        sdr->set_HW_TX_mode(RadioThread::TxMode::TX_70CM);
        sdr->set_HW_TX();
        sleep(1);
        sdr->set_HW_TX_mode(RadioThread::TxMode::TX_DIRECT);
        sdr->set_HW_TX();
        sleep(1);
        sdr->set_HW_RX();
    }



    iqpipe_rx->flush();
    iqpipe_tx->flush();
    delete(sdr);

    // join waiting thread so that it stops properly
    w1.join();

    return 0;
}