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
#include <complex>

#include "lime/LimeSuite.h"
#include "liquid/liquid.h"

#include "util/log.h"

class LimeRadioThread : public RadioThread {
public:

    size_t LMS_Channel = 0;
    

    LimeRadioThread();
    ~LimeRadioThread();

    void run() override;
    void terminate() override;

    void setFrequency(float_t frequency) override;
    void setSamplingRate(float_t sampling_rate, size_t oversampling) override;
    // void getIQData();
    // void setIQData();

private:


    lms_device_t* m_lms_device = NULL;
    lms_stream_t m_rx_streamId;         // RX stream structure
    lms_stream_t m_tx_streamId;         // TX stream structure
    lms_stream_meta_t m_rx_metadata;    // Use metadata for additional control over sample receive function behavior
    lms_stream_meta_t m_tx_metadata;    // Use metadata for additional control over sample receive function behavior


    //data buffers for RX
    const int m_rxSampleCnt = 5000; //complex samples per buffer
    void *m_rxIQbuffer  { nullptr };

    RadioThreadIQDataQueuePtr m_IQdataRXQueue; // = std::make_shared<RadioThreadIQDataQueue>();
    RadioThreadIQDataPtr m_rxIQdataOut;
    
    //data buffers for TX
    const int m_txSampleCnt = 5000; //complex samples per buffer
    void *m_txIQbuffer  { nullptr };

    RadioThreadIQDataQueuePtr m_IQdataTXQueue; // = std::make_shared<RadioThreadIQDataQueue>();
    RadioThreadIQDataPtr m_txIQdataOut;



    int initLimeSDR();
    void closeLimeSDR();
    
    void initStreaming();
    void stopStreaming();

    void printRadioConfig();

    int error();

    

};
