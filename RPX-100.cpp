/******************************************************************************
 * C++ source of RPX-100
 *
 * File:   RPX-100.cpp
 * Author: Bernhard Isemann
 *
 * Created on 01 Jan 2022, 10:35
 * Updated on 15 Apr 2023, 15:20
 * Version 1.00
 *****************************************************************************/

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

#define PORT 8085

#define SPECon true

using json = nlohmann::json;

int main(int argc, const char* argv[])
{
    // init logging for spdlog, select log level: trace, debug, info
    Log::Init(2);
    LOG_APP_INFO("******************************************************************************");

    // init cmdline parsing w/ cxxopts
    cxxopts::Options options("RPX-100", "RPX-100 Backend");

    options
            .set_width(70)
            .add_options()
                    ("h,help", "Print help")
                    ("c,config", "Config file")
                    ("s", "websocket for spectrogram")
                    ("p", "phy testing")
                    ("l", "gpio test")
            ;

    auto result = options.parse(argc, argv);

    if(result.count("help")) {
        std::cout << options.help({""}) << std::endl;
        exit(0);
    }

    LOG_APP_INFO("*                       RPX-100 Backend started                              *");
    LOG_APP_INFO("******************************************************************************");


    // read config file
    std::ifstream SystemConfigFile("SystemConfig.json");
    json SystemConfig = json::parse(SystemConfigFile);
    SystemConfigFile.close();

    float_t cf_samp_rate = SystemConfig["Radio"]["SAMPLING_RATE"];
    size_t cf_oversampling = SystemConfig["Radio"]["OVERSAMPLING"];
    float_t cf_center_freq = SystemConfig["Radio"]["CENTER_FREQ"];

    // create SDR object
    RadioThread *sdr;

    // create RX and TX queues to communicate with the SDR object
    RadioThreadIQDataQueuePtr iqpipe_rx = std::make_shared<RadioThreadIQDataQueue>();
    iqpipe_rx->set_max_items(2000);

    RadioThreadIQDataQueuePtr iqpipe_tx = std::make_shared<RadioThreadIQDataQueue>();
    iqpipe_tx->set_max_items(2000);

    // init SDR
    // sdr = new LimeRadioThread();  // default is defined via DEFAULT_SAMPLEBUFFERCNT
    sdr = new LimeRadioThread(3200);  // set to a specifc sampleCnt (# samples RX , # samples TX max)
    sdr->setRXQueue(iqpipe_rx);
    sdr->setTXQueue(iqpipe_tx);
    sdr->setFrequency(cf_center_freq);
    sdr->setSamplingRate(cf_samp_rate, cf_oversampling);


    // create SDR Thread
    std::thread *t_sdr = nullptr;

    //Start RadioThread this is also starting the RX and TX streams
    t_sdr = new std::thread(&RadioThread::threadMain, sdr);

    if(result.count("s")) {
        // Start websocket server with IQ stream
        wsSpectrogram *wsspec;
        wsspec = new wsSpectrogram(PORT);
        LOG_APP_INFO("Started WebSocketServer on Port 8085");
        wsspec->setQueue(iqpipe_rx);
        std::thread *t_wsspec = nullptr;
        t_wsspec = new std::thread(&wsSpectrogram::threadMain, wsspec);
        while(SPECon) {
            //LOG_APP_INFO("Running");
        }
    }

    if(result.count("p")) {
        PhyThread *phy;
        phy = new PhyThread(1);
        LOG_APP_INFO("PHY Layer running");
        phy->setRXQueue(iqpipe_rx);
        LOG_APP_INFO("RXQueue");
        phy->run(); // this is blocking for testing at the moment

    }


    // test GPIOS w/ connected LED's /w cmdline -l option
    if(result.count("l")) {
        LOG_APP_INFO("Start GPIO testing");
        sdr->set_HW_SDR_ON();
        sleep(5);
        sdr->set_HW_RX();
        sleep(5);
        sdr->set_HW_TX(RadioThread::TxMode::TX_DIRECT);
        sleep(5);
        sdr->set_HW_TX(RadioThread::TxMode::TX_6M);
        sleep(5);
        sdr->set_HW_TX(RadioThread::TxMode::TX_2M);
        sleep(5);
        sdr->set_HW_TX(RadioThread::TxMode::TX_70CM);
        sleep(5);
        sdr->set_HW_RX();
        sleep(5);
        sdr->set_HW_TX(RadioThread::TxMode::TX_70CM);
        sleep(5);
        sdr->set_HW_TX(RadioThread::TxMode::TX_2M);
        sleep(5);
        sdr->set_HW_TX(RadioThread::TxMode::TX_6M);
        sleep(5);
        sdr->set_HW_TX(RadioThread::TxMode::TX_DIRECT);
        sleep(5);
        sdr->set_HW_RX();
        LOG_APP_INFO("GPIO testing completed");
    }

    iqpipe_rx->flush();
    iqpipe_tx->flush();
    sdr->terminate();
    delete(sdr);

    return 0;
}
