#pragma once

#include <mutex>
#include <atomic>
#include <deque>
#include <map>
#include <set>
#include <string>
#include <iostream>
#include <thread>
#include <memory>
#include <climits>
#include <vector>
#include <deque>
#include <complex>

#include "lime/LimeSuite.h"
#include "liquid/liquid.h"
#include "phy/DefaultRadioConfig.h"

#include "util/log.h"


// for some stuff around the iq queue handling ideas from CubicSDR were taken  
// CubuicSDR (c) Charles J. Cliffe

class SpinMutex {

public:
    SpinMutex() = default;

    SpinMutex(const SpinMutex&) = delete;

    SpinMutex& operator=(const SpinMutex&) = delete;

    ~SpinMutex() { lock_state.clear(std::memory_order_release); }

    void lock() { while (lock_state.test_and_set(std::memory_order_acquire)); }

    bool try_lock() {return !lock_state.test_and_set(std::memory_order_acquire); }

    void unlock() { lock_state.clear(std::memory_order_release); }

private:
    std::atomic_flag lock_state = ATOMIC_FLAG_INIT;
};

struct map_string_less
{
    bool operator()(const std::string& a,const std::string& b) const
    {
        return a.compare(b) < 0;
    }
};

/**
 * RadioThreadIQData class
 *
 * @note one block of received IQ data with timestamp of first sample of block; data is stored in liquid_float_complex
 *
 */
class RadioThreadIQData {
public:
    long long frequency;
    long long sampleRate;

    uint64_t timestampFirstSample;

    std::vector<liquid_float_complex> data;   //std::complex<float>

    RadioThreadIQData() :
            frequency(DEFAULT_CENTER_FREQ), sampleRate(DEFAULT_SAMPLE_RATE), timestampFirstSample(0) {
    
//        LOG_RADIO_DEBUG("RadioThreadIQData() constructor");

    }

    virtual ~RadioThreadIQData() = default;
};

typedef std::shared_ptr<RadioThreadIQData> RadioThreadIQDataPtr;



class ThreadIQDataQueueBase {
};

typedef std::shared_ptr<ThreadIQDataQueueBase> ThreadIQDataQueueBasePtr;

/**
 * RadioThreadIQDataQueue class
 *
 * @note for storing IQ sample blocks - currently more a quick hack to get IQ data to verify it
 *
 */
class RadioThreadIQDataQueue : public ThreadIQDataQueueBase {
public:

    typedef typename std::deque<RadioThreadIQDataPtr>::value_type value_type;
    typedef typename std::deque<RadioThreadIQDataPtr>::size_type size_type;

    RadioThreadIQDataQueue() { 
        // constructor
        LOG_RADIO_DEBUG("RadioThreadIQDataQueue() constructor");

    }

    void set_max_items(unsigned int max_items) {
        std::lock_guard < SpinMutex > lock(m_mutex);

        LOG_RADIO_DEBUG("set_max_items() {}", max_items);

        if (max_items > m_max_items) {
            m_max_items = max_items;
        }
    }

    bool push(const value_type& item) {
        std::unique_lock < SpinMutex > lock(m_mutex);

        //LOG_RADIO_DEBUG("push() size queue {} - max {}", iq_queue.size(), m_max_items);

        if(m_iq_queue.size() >= m_max_items) {
            return false;
            LOG_RADIO_ERROR("push() queue full");
        }

        m_iq_queue.push_back(item);
        return true;
    }

    bool pop(value_type& item) {
        std::unique_lock < SpinMutex > lock(m_mutex);

        if(m_iq_queue.size() > 0) {
            item = m_iq_queue.front();
            m_iq_queue.pop_front();
            return true;
        }

        return false;
    }

    void flush() {
        std::lock_guard < SpinMutex > lock(m_mutex);
        m_iq_queue.clear();
    }

    size_type size() const {
        std::lock_guard < SpinMutex > lock(m_mutex);
        return m_iq_queue.size();
    }

    void print_size() {
        LOG_RADIO_DEBUG("RadioThreadIQDataQueue() size queue {}", m_iq_queue.size());
    }


    ~RadioThreadIQDataQueue() { 
        LOG_RADIO_DEBUG("RadioThreadIQDataQueue() de-constructor");
    }

private:

    std::deque<RadioThreadIQDataPtr> m_iq_queue;
    mutable SpinMutex m_mutex;

    size_t m_max_items = 1; // default value for max items

};

typedef std::shared_ptr<RadioThreadIQDataQueue> RadioThreadIQDataQueuePtr;


class RadioThread {
public:
    RadioThread();
    virtual ~RadioThread();

    //the thread Main call back itself
    virtual void threadMain();

    virtual void setup();
    virtual void run();

    //Request for termination (asynchronous)
    virtual void terminate();

    //Returns true if the thread is indeed terminated, i.e the run() method has returned. 
    bool isTerminated(int waitMs = 0);

    void setRXQueue(const ThreadIQDataQueueBasePtr& threadQueue);
    ThreadIQDataQueueBasePtr getRXQueue();

    void setTXQueue(const ThreadIQDataQueueBasePtr& threadQueue);
    ThreadIQDataQueueBasePtr getTXQueue();

    // SDR Radio stuff as virutal functions which are then defined in the specifc radio class

    /**
     * @brief checks if LMSReceive is running in run()
     * 
     * @return true 
     * @return false 
     */
    bool isReceiverRunning() {
        return m_isReceiverRunning.load();
    }

    void setRxOn() {
        m_isRX.store(true); // true = RX
    }

    void setTxOn() {
        m_isRX.store(false); // true = TX
    }


    /**
     * Set RF center frequency in Hz.
     *
     * @param   frequency   Desired RF center frequency in Hz
     *
     * @return  void
     */
    virtual void setFrequency(float_t frequency);

    /**
     * @brief Set the Sampling_Rate and Oversampling
     * 
     * @param sampling_rate (in Hz)
     * @param oversampling (0,2,4,8)
     */
    virtual void setSamplingRate(float_t sampling_rate, size_t oversampling);

    // done via queues now
    // virtual void getIQData();
    // virtual void setIQData();
    
protected:
    ThreadIQDataQueueBasePtr m_tx_queue;
    ThreadIQDataQueueBasePtr m_rx_queue;

    std::mutex m_queue_bindings_mutex;

    std::atomic_bool stopping;

    std::atomic_bool m_isReceiverRunning;

    /**
     * @brief m_isRX is true when the RadioThread is in RX mode; if false the thread is running in TX
     * 
     */
    std::atomic_bool m_isRX;

private:
    //true when the thread has really ended, i.e run() from threadMain() has returned.
    std::atomic_bool terminated;

};

