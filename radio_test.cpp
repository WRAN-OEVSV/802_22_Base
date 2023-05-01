
#include <memory>
#include <thread>
#include <chrono>
#include <iostream>
#include <fstream> 
#include <complex>

#include "lime/LimeSuite.h"
#include "liquid/liquid.h"
#include "util/log.h"
#include "util/ws_spectrogram.h"
#include "util/write_csv_file.h"

#include <nlohmann/json.hpp>
#include <cxxopts.hpp>

#include "phy/RadioThread.h"
#include "phy/LimeRadioThread.h"
#include "phy/PhyThread.h"


using json = nlohmann::json;

// #####################################################
// in ../build => g++ ../*.cpp ../phy/*.cpp -I.. -I../phy -I../external/spdlog/include -std=c++17 -pthread -lLimeSuite -L/usr/lib -o radio_test
// use cmake now!!!


void warta(PhyThread *r, int64_t t) {

    // start to wait when RadioThread is collecting data
    while(!r->isPhyRunning()) {
        std::cout << "x";
    }

    LOG_TEST_INFO("warta() for {} sec", t);
    std::this_thread::sleep_for(std::chrono::seconds(t));

    LOG_TEST_INFO("warta() done sleeping");
    r->stop();

    std::cerr << "sdr stop" << std::endl;
}



int main(int argc, const char* argv[])
{

    // some generic system init must be implement which is setting up all the "utilities" stuff
    // init logging for spdlog
    Log::Init(0, "RPX-100.log");

    // init cmdline parsing w/ cxxopts
    cxxopts::Options options("radio_test", "Test PHY SDR RX / TX");

    options
      .set_width(70)
      .add_options()
      ("h,help", "Print help")
      ("c,config", "Config file")
      ("w", "write CSV file")
      ("s", "websocket for spectrogram")
      ("p", "phy testing")
      ("b", "phy basestation")
      ("l", "gpio test")
    ;

    auto result = options.parse(argc, argv);

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


    // websocket spectrogram test stuff
    if(result.count("s")) {

        // create SDR object
        RadioThread *sdr;

        // create RX and TX queues to communicate with the SDR object
        RadioThreadIQDataQueuePtr iqpipe_rx = std::make_shared<RadioThreadIQDataQueue>();
        iqpipe_rx->set_max_items(2000);

        RadioThreadIQDataQueuePtr iqpipe_tx = std::make_shared<RadioThreadIQDataQueue>();
        iqpipe_tx->set_max_items(2000);

        // init SDR
    //    sdr = new LimeRadioThread();  // default is defined via DEFAULT_SAMPLEBUFFERCNT
        sdr = new LimeRadioThread(3200);  // set to a specifc sampleCnt (# samples RX , # samples TX max)
        sdr->setRXQueue(iqpipe_rx);
        sdr->setTXQueue(iqpipe_tx);
        sdr->setFrequency(cf_center_freq);
        sdr->setSamplingRate(cf_samp_rate, cf_oversampling);

        // create SDR Thread
        std::thread *t_sdr = nullptr;

        //Start RadioThread this is also starting the RX and TX streams
        t_sdr = new std::thread(&RadioThread::threadMain, sdr);


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

        wsSpectrogram *wsspec;
        wsspec = new wsSpectrogram(9123);
        wsspec->setQueue(iqpipe_rx);

        // create Thread
        std::thread *t_wsspec = nullptr;

        //Start Thread
        t_wsspec = new std::thread(&wsSpectrogram::threadMain, wsspec);


        while(!wsspec->isWsRunning()) {
            std::cout << "w";
        }

        // @todo stop command via WS 
    
        // be nice and clean up
        delete(wsspec);
        iqpipe_rx->flush();
        iqpipe_tx->flush();
        delete(t_sdr);
        
    } 

 
    // write data for debug to ramdisk (cmdline -w option) - anyways  is even with ram very slow ...
    // root@x:~# mkdir /tmp/ramdisk
    // root@x:~# chmod 777 ramdisk/
    // root@x:~# mount -t tmpfs -o size=200M myramdisk /tmp/ramdisk/

    // bool stop = true;

    // if(result.count("w")) {
    //     write_csv_file(sdr, iqpipe_rx,"/tmp/ramdisk/IQData.csv");
    //     stop = false;
    // }
    
    if(result.count("p")) {
        PhyThread *phy;
        if(result.count("b")) {
            phy = new PhyThread(PhyThread::PhyMode::BASESTATION, cf_samp_rate, cf_oversampling, cf_center_freq);
        } else {
            phy = new PhyThread(PhyThread::PhyMode::CPE, cf_samp_rate, cf_oversampling, cf_center_freq);
        }

        //wait for waitTime seconds and then stop the phy thread to limit the amount
        //of data collected for testing
        uint64_t waitTime = 20;
        std::thread w1(warta,phy, waitTime);
 
        phy->run(); // this is blocking for testing at the moment
 
        // join waiting thread so that it stops properly
        w1.join();

        // be nice and clean up
        delete(phy);
    }


    // // test GPIOS w/ connected LED's /w cmdline -l option
    // if(result.count("l")) {

    //     sdr->set_HW_RX();
    //     sleep(1);
    //     sdr->set_HW_TX(RadioThread::TxMode::TX_DIRECT);
    //     sleep(1);
    //     sdr->set_HW_TX(RadioThread::TxMode::TX_6M);
    //     sleep(1);
    //     sdr->set_HW_TX(RadioThread::TxMode::TX_2M);
    //     sleep(1);
    //     sdr->set_HW_TX(RadioThread::TxMode::TX_70CM);
    //     sleep(1);
    //     sdr->set_HW_RX();
    //     sleep(1);
    //     sdr->set_HW_TX();
    //     sleep(1);
    //     sdr->set_HW_TX_mode(RadioThread::TxMode::TX_6M);
    //     sdr->set_HW_TX();
    //     sleep(1);
    //     sdr->set_HW_TX_mode(RadioThread::TxMode::TX_2M);
    //     sdr->set_HW_TX();
    //     sleep(1);
    //     sdr->set_HW_TX_mode(RadioThread::TxMode::TX_70CM);
    //     sdr->set_HW_TX();
    //     sleep(1);
    //     sdr->set_HW_TX_mode(RadioThread::TxMode::TX_DIRECT);
    //     sdr->set_HW_TX();
    //     sleep(1);
    //     sdr->set_HW_RX();
    // }


    return 0;
}