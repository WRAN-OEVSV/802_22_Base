
#include "phy/PhyThread.h"


PhyThread::PhyThread(int phyMode) : m_phyMode{phyMode}, m_currentSampleTimestamp{0} {
    LOG_PHY_INFO("PhyThread::PhyThread() constructor - phy is runing in mode {}", phyMode);
    terminated.store(false);
    stopping.store(false);

    m_frameSync = PhyFrameSync(PHY_SUBCARRIERS__M, PHY_CP_STS_LTS, PHY_TAPERLEN);

}

PhyThread::~PhyThread() {
    LOG_PHY_INFO("PhyThread::~PhyThread() de-constructor");
    terminated.store(true);
    stopping.store(true);
}

void PhyThread::threadMain() {
    terminated.store(false);
    stopping.store(false);
    try {
        run();
    }
    catch (...) {
        terminated.store(true);
        stopping.store(true);
        throw;
    }
  
    terminated.store(true);
    stopping.store(true);
}

void PhyThread::terminate() {
    stopping.store(true);
}



void PhyThread::run() {
    // do signal processing magic here

    LOG_PHY_INFO("PHY thread starting.");

    m_isPhyRunning.store(true);

    m_IQdataRXQueue = std::static_pointer_cast<RadioThreadIQDataQueue>( PhyThread::getRXQueue());
    m_IQdataTXQueue = std::static_pointer_cast<RadioThreadIQDataQueue>( PhyThread::getTXQueue());


    while(!stopping)
    {

        m_rxIQdataOut = std::make_shared<RadioThreadIQData>();

        if(m_IQdataRXQueue->size() > 5) {

            m_IQdataRXQueue->pop(m_rxIQdataOut);

            m_currentSampleTimestamp = m_rxIQdataOut->timestampFirstSample;

            for(auto sample: m_rxIQdataOut->data)
            {
                m_frameSync.m_currentSampleTimestamp = m_currentSampleTimestamp;
                m_frameSync.execute(sample);
                m_currentSampleTimestamp++;
            }

        }
    }
}




bool PhyThread::phyConfig(/*parameter*/) {

//  // validate input
//     if (_M < 8)
//         return liquid_error_config("ofdmflexframesync_create(), less than 8 subcarriers");
//     if (_M % 2)
//         return liquid_error_config("ofdmflexframesync_create(), number of subcarriers must be even");
//     if (_cp_len > _M)
//         return liquid_error_config("ofdmflexframesync_create(), cyclic prefix length cannot exceed number of subcarriers");

    return true;
}






void PhyThread::setRXQueue(const ThreadIQDataQueueBasePtr& threadQueue) {
    std::lock_guard < std::mutex > lock(m_queue_bindings_mutex);
    LOG_PHY_DEBUG("PhyThread::setRXQueue()");
    m_rx_queue = threadQueue;
}

ThreadIQDataQueueBasePtr PhyThread::getRXQueue() {
    std::lock_guard < std::mutex > lock(m_queue_bindings_mutex);
    LOG_PHY_DEBUG("PhyThread::getRXQueue() ");
    return m_rx_queue;
}

void PhyThread::setTXQueue(const ThreadIQDataQueueBasePtr& threadQueue) {
    std::lock_guard < std::mutex > lock(m_queue_bindings_mutex);
    LOG_PHY_DEBUG("PhyThread::setTXQueue()");
    m_tx_queue = threadQueue;
}

ThreadIQDataQueueBasePtr PhyThread::getTXQueue() {
    std::lock_guard < std::mutex > lock(m_queue_bindings_mutex);
    LOG_PHY_DEBUG("PhyThread::getTXQueue() ");
    return m_tx_queue;
}
