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

// class SpinMutex {

// public:
//     SpinMutex() = default;

//     SpinMutex(const SpinMutex&) = delete;

//     SpinMutex& operator=(const SpinMutex&) = delete;

//     ~SpinMutex() { lock_state.clear(std::memory_order_release); }

//     void lock() { while (lock_state.test_and_set(std::memory_order_acquire)); }

//     bool try_lock() {return !lock_state.test_and_set(std::memory_order_acquire); }

//     void unlock() { lock_state.clear(std::memory_order_release); }

// private:
//     std::atomic_flag lock_state = ATOMIC_FLAG_INIT;
// };

// struct map_string_less
// {
//     bool operator()(const std::string& a,const std::string& b) const
//     {
//         return a.compare(b) < 0;
//     }
// };

/**
 * RadioIQData class
 *
 * @note one block of received IQ data with timestamp of first sample of block; data is stored in liquid_float_complex
 *
 */
class RadioIQData {
public:
    long long frequency;
    long long sampleRate;

    uint64_t timestampFirstSample;

    std::deque<liquid_float_complex> data;   //std::complex<float>

    RadioIQData() :
            frequency(DEFAULT_CENTER_FREQ), sampleRate(DEFAULT_SAMPLE_RATE), timestampFirstSample(0) {
        LOG_RADIO_DEBUG("RadioIQData::RadioIQData() constructor");
    }

    virtual ~RadioIQData() = default;
};

typedef std::shared_ptr<RadioIQData> RadioIQDataPtr;


class Radio {
public:
    Radio();
    Radio(int sampleBufferCnt);
    virtual ~Radio();


    virtual void setup();

    // SDR Radio stuff as virutal functions which are then defined in the specifc radio class

    /**
     * @brief checks if LMSReceive is running in run()
     * 
     * @return true 
     * @return false 
     */
    bool isRxTxRunning() {
        return m_isRxTxRunning.load();
    }

    void setRxOn() {
        m_isRX.store(true); // true = RX
    }

    void setTxOn() {
        m_isRX.store(false); // true = TX
    }


    void setRXBuffer(const RadioIQDataPtr& buffer);
    RadioIQDataPtr getRXBuffer();

    void setTXBuffer(const RadioIQDataPtr& buffer);
    RadioIQDataPtr getTXBuffer();



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

    //Transmit modes
    typedef enum
    {
        TX_DIRECT=1,
        TX_6M,
        TX_2M,
        TX_70CM
    } TxMode;

    /**
     * @brief set HW GPIO for RX
     * 
     */
    virtual void set_HW_RX();

    /**
     * @brief set HW GPIO for TX - used m_TxMode set via set_HW_TX_mode
     * 
     */
    virtual void set_HW_TX() { set_HW_TX(m_TxMode); };

    /**
     * @brief set HW GPIO for TX
     * 
     * @param m mode based on RadioThread::TxMode enum
     */
    virtual void set_HW_TX(TxMode m);

    virtual void set_HW_TX_mode(TxMode m) { m_TxMode = m; };



    virtual int receive_IQ_data();

    virtual int send_IQ_data();

    virtual int send_Tone();

    virtual uint64_t get_sample_timestamp();



    lms_stream_status_t m_rx_status;    // status of RX stream from LMS_GetStatus   (is updated on receive and get timestamp)
    lms_stream_status_t m_tx_status;    // status of TX stream from LMS_GetStatus   (is updated on transmit and get timestamp)

    
protected:

    RadioIQDataPtr m_tx_buffer;
    RadioIQDataPtr m_rx_buffer;


    std::mutex m_queue_bindings_mutex;

    std::atomic_bool m_isRxTxRunning;

    /**
     * @brief m_isRX is true when the RadioThread is in RX mode; if false the thread is running in TX
     * 
     */
    std::atomic_bool m_isRX;

    int m_sampleBufferCnt;

private:

    TxMode  m_TxMode = TxMode::TX_DIRECT;

};

