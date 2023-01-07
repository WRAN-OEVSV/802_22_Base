#include "phy/RadioThread.h"
#include "phy/LimeRadioThread.h"
#include "lime/LimeSuite.h"
#include "liquid/liquid.h"
#include <memory>
#include <thread>
#include <chrono>
#include <iostream>
#include <fstream> 
#include <complex>

// #####################################################
// in ../build => g++ ../*.cpp ../phy/*.cpp -I.. -I../phy -I../external/spdlog/include -std=c++17 -pthread -lLimeSuite -L/usr/lib -o radio_test
// use cmake now!!!

// test program reading 2 sec of IQ data using a thread 
// RadioThread run() is reading the 2 sec data

// getIQData is currently writing data into CSV file for futher
// analysis with octave

int main()
{

    Log::Init();

    LOG_RADIO_INFO("SDR Radio test program");

    RadioThread *sdr;
    std::thread *t_sdr = nullptr;

    sdr = new LimeRadioThread;
    
   // sdr->setFrequency(52e6);

    //Start RadioThread 
    t_sdr = new std::thread(&RadioThread::threadMain, sdr);

    
    // do something while thread is running ..
    for(int i=0; i<100; i++)
        std::cout << ".";
    std::cout << std::endl;
    sdr->setIQData();

    // sich an den thread anhängen bis dieser fertig ist
    t_sdr->join();

    //sdr->getIQData();
    


    delete(sdr);

    return 0;
}