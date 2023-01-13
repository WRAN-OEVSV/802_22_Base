#include "phy/RadioThread.h"
#include "lime/LimeSuite.h"
#include "liquid/liquid.h"
#include <memory>
#include <thread>
#include <chrono>
#include <iostream>
#include <fstream> 
#include <complex>


// for some stuff around the iq queue handling ideas from CubicSDR were taken 
// CubuicSDR (c) Charles J. Cliffe

#define SPIN_WAIT_SLEEP_MS 5

RadioThread::RadioThread() {
    LOG_RADIO_DEBUG("RadioThread() constructor");
    terminated.store(false);
    stopping.store(false);
    m_isRxTxRunning.store(false);
    m_isRX.store(true);                     // default is to run in RX mode
}

RadioThread::~RadioThread() {
    LOG_RADIO_DEBUG("RadioThread() de-constructor");
    terminated.store(true);
    stopping.store(true);
}

void RadioThread::threadMain() {
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

void RadioThread::setup() {
    // defined in radio specific class (e.g. LimeRadioThread)
}

void RadioThread::run() {
    // defined in radio specific class (e.g. LimeRadioThread)
}


void RadioThread::terminate() {
    stopping.store(true);
}

void RadioThread::stop() {
    stopping.store(true);
}


bool RadioThread::isTerminated(int waitMs) {

    if (terminated.load()) {
        return true;
    }
    else if (waitMs == 0) {
        return false;
    }

    //this is a stupid busy plus sleep loop
    int nbCyclesToWait;

    if (waitMs < 0) {
        nbCyclesToWait = std::numeric_limits<int>::max();
    }
    else {
        nbCyclesToWait = (waitMs / SPIN_WAIT_SLEEP_MS) + 1;
    }

    for ( int i = 0; i < nbCyclesToWait; i++) {

        std::this_thread::sleep_for(std::chrono::milliseconds(SPIN_WAIT_SLEEP_MS));

        if (terminated.load()) {
            return true;
        }
    }

    LOG_RADIO_ERROR("ERROR: thread {} has not terminated in time ! (> {}  ms)<< ", typeid(*this).name() , waitMs );

    return terminated.load();
}


void RadioThread::setFrequency(float_t frequency) {
    // defined in radio specific class (e.g. LimeRadioThread)
}


void RadioThread::setSamplingRate(float_t sampling_rate, size_t oversampling) {
    // defined in radio specific class (e.g. LimeRadioThread)
}

// void RadioThread::getIQData()
// {
//     // defined in radio specific class (e.g. LimeRadioThread)
// }

// void RadioThread::setIQData()
// {
//     // defined in radio specific class (e.g. LimeRadioThread)
// }

void RadioThread::set_HW_RX() {
    // defined in radio specific class (e.g. LimeRadioThread)
};
void RadioThread::set_HW_TX(TxMode m) {
    // defined in radio specific class (e.g. LimeRadioThread)
};

void RadioThread::setRXQueue(const ThreadIQDataQueueBasePtr& threadQueue) {
    std::lock_guard < std::mutex > lock(m_queue_bindings_mutex);
    LOG_RADIO_DEBUG("setRXQueue()");
    m_rx_queue = threadQueue;
}

ThreadIQDataQueueBasePtr RadioThread::getRXQueue() {
    std::lock_guard < std::mutex > lock(m_queue_bindings_mutex);
    LOG_RADIO_DEBUG("getRXQueue() ");
    return m_rx_queue;
}

void RadioThread::setTXQueue(const ThreadIQDataQueueBasePtr& threadQueue) {
    std::lock_guard < std::mutex > lock(m_queue_bindings_mutex);
    LOG_RADIO_DEBUG("setTXQueue()");
    m_tx_queue = threadQueue;
}

ThreadIQDataQueueBasePtr RadioThread::getTXQueue() {
    std::lock_guard < std::mutex > lock(m_queue_bindings_mutex);
    LOG_RADIO_DEBUG("getTXQueue() ");
    return m_tx_queue;
}
