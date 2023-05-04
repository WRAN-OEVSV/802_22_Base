#pragma once

#include <complex>
#include <iostream>
#include <fstream>
#include <chrono>
#include <unistd.h>

#include "liquid/liquid.h"

#include "phy/RadioThread.h"
#include "phy/Radio.h"
#include "phy/LimeRadio.h"
#include "phy/PhyFrameSync.h"
#include "phy/PhyFrameGen.h"
#include "phy/PhyDefinitions.h"

#include "phy/PhyIQDebug.h"

#include "util/log.h"

/**
 * @brief class is managing Timing information of the Phy e.g. NCO for other information
 * 
 */
class PhyTiminig {


// NCO is relevant for the CPE side; to control any carrier drift and respective lock to the BS carrier


};



class PhyThread {
public:

    typedef enum
    {
        BASESTATION=0,
        CPE,
        TEST
    } PhyMode;

    PhyThread(PhyMode mode);

    PhyThread(PhyMode mode, float_t samp_rate, size_t oversampling, float_t center_freq);


    ~PhyThread();

    //the thread Main call back itself
    void threadMain();

    void run();

    //Request for termination (asynchronous)
    void terminate();


    bool phyConfig(/*parameter*/);


    // keep keep RX and TX for the direction sync with the SDR i.e. RXQueue for PHY is what SDR received
    // this could then be either US or DS depending on what mode the Phy is running (BS or CPE)
    void setRXQueue(const ThreadIQDataQueueBasePtr& threadQueue);
    ThreadIQDataQueueBasePtr getRXQueue();

    void setTXQueue(const ThreadIQDataQueueBasePtr& threadQueue);
    ThreadIQDataQueueBasePtr getTXQueue();


    bool isPhyRunning() {
        return m_isPhyRunning.load();
    }

    void stop() {
        stopping.store(true);
    }


    PhyIQDebugPtr getIQDebug() { return m_iqdebug; }


protected:

    ThreadIQDataQueueBasePtr m_tx_queue;
    ThreadIQDataQueueBasePtr m_rx_queue;

    RadioIQDataPtr m_iqbuffer_rx = std::make_shared<RadioIQData>();
    RadioIQDataPtr m_iqbuffer_tx = std::make_shared<RadioIQData>();


    std::atomic_bool stopping;

    std::mutex m_queue_bindings_mutex;


    std::atomic_bool m_isPhyRunning;

private:


    // thread related variables
    std::atomic_bool terminated;


    // sdr related variables
    Radio *m_sdrRadio;      // create in constructor
    float_t m_samp_rate;
    size_t m_oversampling;
    float_t m_center_freq;
    uint64_t m_currentSampleTimestamp;



    PhyFrameSync m_frameSync;
    PhyFrameGen m_frameGen;

    uint64_t    m_framestart_timestamp;

    /**
     * @brief phyMode is either 0 for BaseStation; 1 for CPE; or other future modes
     * @brief default the Phy is running as BaseStation; mode cannot be changed on runtime
     * 
     */
    const PhyMode m_phyMode;


    RadioThreadIQDataQueuePtr m_IQdataRXQueue; // = std::make_shared<RadioThreadIQDataQueue>();
    RadioThreadIQDataPtr m_rxIQdataOut;

    RadioThreadIQDataQueuePtr m_IQdataTXQueue; // = std::make_shared<RadioThreadIQDataQueue>();
    RadioThreadIQDataPtr m_txIQdataOut;

    PhyIQDebugPtr m_iqdebug;

};