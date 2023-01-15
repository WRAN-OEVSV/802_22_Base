#pragma once

#include <complex>
#include <iostream>
#include <fstream>

#include "liquid.h"

#include "phy/RadioThread.h"
#include "phy/PhyFrameSync.h"
#include "phy/PhyDefinitions.h"


#include "util/log.h"



class PhyThread {
public:


    PhyThread(int phyMode);

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


protected:

    ThreadIQDataQueueBasePtr m_tx_queue;
    ThreadIQDataQueueBasePtr m_rx_queue;

    std::atomic_bool stopping;

    std::mutex m_queue_bindings_mutex;


    std::atomic_bool m_isPhyRunning;

private:


    PhyFrameSync m_frameSync;

    std::atomic_bool terminated;

    /**
     * @brief phyMode is either 0 for BaseStation; 1 for CPE; or other future modes
     * @brief default the Phy is running as BaseStation; mode cannot be changed on runtime
     * 
     */
    const int m_phyMode;


    RadioThreadIQDataQueuePtr m_IQdataRXQueue; // = std::make_shared<RadioThreadIQDataQueue>();
    RadioThreadIQDataPtr m_rxIQdataOut;

    RadioThreadIQDataQueuePtr m_IQdataTXQueue; // = std::make_shared<RadioThreadIQDataQueue>();
    RadioThreadIQDataPtr m_txIQdataOut;

};