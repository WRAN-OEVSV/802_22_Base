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

    void setFrequency(double frequency) override;
    // void getIQData();
    // void setIQData();

private:


    lms_device_t* m_lms_device = NULL;
    lms_stream_t m_rx_streamId; //stream structure
    lms_stream_meta_t m_rx_metadata; //Use metadata for additional control over sample receive function behavior


    //data buffers
    const int sampleCnt = 5000; //complex samples per buffer
    void *IQbuffer  { nullptr };


    RadioThreadIQDataQueuePtr IQdataOutQueue; // = std::make_shared<RadioThreadIQDataQueue>();
    RadioThreadIQDataPtr IQdataOut;
    
    int initLimeSDR();
    void closeLimeSDR();
    
    void initStreaming();
    void stopStreaming();

    int error();

    

};
