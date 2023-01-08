
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


// #####################################################
// in ../build => g++ ../*.cpp ../phy/*.cpp -I.. -I../phy -I../external/spdlog/include -std=c++17 -pthread -lLimeSuite -L/usr/lib -o radio_test
// use cmake now!!!

// test program reading 2 sec of IQ data using a thread 
// RadioThread run() is reading the 2 sec data

// program is currently writing data into CSV file for futher
// analysis with octave

int main()
{

    // some generic system init must be implement which is setting up all the "utilities" stuff
    Log::Init();

    LOG_TEST_INFO("SDR Radio test program");

    RadioThread *sdr_rx;
    std::thread *t_sdr = nullptr;

    RadioThreadIQDataQueuePtr iqpipe_rx = std::make_shared<RadioThreadIQDataQueue>();
    iqpipe_rx->set_max_items(1000);


    sdr_rx = new LimeRadioThread;

    sdr_rx->setRXQueue(iqpipe_rx);
    
    //sdr->setFrequency(52e6);

    //Start RadioThread 
    t_sdr = new std::thread(&RadioThread::threadMain, sdr_rx);

    
    // sich an den thread anhängen bis dieser fertig ist
    // t_sdr->join();

    //sdr->getIQData();

    LOG_TEST_INFO("wait for receiver");
    while(!sdr_rx->isReceiverRunning()) {
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


    // write data for debug to ramdisk - anyways disk is even with ram very slow ...
    // root@x:~# mkdir /tmp/ramdisk
    // root@x:~# chmod 777 ramdisk/
    // root@x:~# mount -t tmpfs -o size=200M myramdisk /tmp/ramdisk/

    std::ofstream myfile("/tmp/ramdisk/IQData.csv");


    // error checking if file is there ... maybe a bit overkill for a test script :-)
    if(!myfile.is_open()) {
        LOG_TEST_ERROR("cannot open /tmp/ramdisk/IQData.csv");

        LOG_TEST_ERROR("terminate thread and flush data");
        sdr_rx->terminate();
        while(!sdr_rx->isTerminated(100)) {
            //flush the iq queue so that it does not overrun
            LOG_TEST_ERROR("wait to terminate");
        }
        LOG_TEST_ERROR("thread is terminated");
    } else {

        RadioThreadIQDataPtr iq;
        
        LOG_TEST_INFO("get data from queue and write to /tmp/ramdisk/IQData.csv ");
        while(sdr_rx->isReceiverRunning()) {

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

    iqpipe_rx->flush();
    delete(sdr_rx);

    return 0;
}