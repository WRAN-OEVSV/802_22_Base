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

#include "phy/Radio.h"

#include "util/log.h"

class LimeRadio : public Radio {
public:

    size_t LMS_Channel = 0;
    
    LimeRadio();
    LimeRadio(int sampleBufferCnt);
    ~LimeRadio();

    int receive_IQ_data() override;
    int send_IQ_data() override;

    int send_Tone() override;

    uint64_t get_sample_timestamp() override;

    void setFrequency(float_t frequency) override;
    void setSamplingRate(float_t sampling_rate, size_t oversampling) override;

    void set_HW_SDR_ON();
    void set_HW_SDR_OFF();
    void set_HW_RX();
    void set_HW_TX(TxMode m);



    // lms_stream_status_t m_rx_status;    // status of RX stream from LMS_GetStatus   (is updated on receive and get timestamp)
    // lms_stream_status_t m_tx_status;    // status of TX stream from LMS_GetStatus   (is updated on transmit and get timestamp)


private:

    lms_device_t* m_lms_device = NULL;
    lms_stream_t m_rx_streamId;         // RX stream structure
    lms_stream_t m_tx_streamId;         // TX stream structure
    lms_stream_meta_t m_rx_metadata;    // Use metadata for additional control over sample receive function behavior
    lms_stream_meta_t m_tx_metadata;    // Use metadata for additional control over sample receive function behavior



    //data buffers for RX
    const int m_rxSampleCnt; //complex samples per buffer is set via constructor
    void *m_rxIQbuffer  { nullptr };

    RadioIQDataPtr m_IQdataRXBuffer;
    
    //data buffers for TX
    const int m_txSampleCnt; //complex samples per buffer is set via constructor
    void *m_txIQbuffer  { nullptr };

    RadioIQDataPtr m_IQdataTXBuffer;

    int initLimeSDR();
    void closeLimeSDR();
    
    void initStreaming();
    void stopStreaming();

    void printRadioConfig();

    int initLimeGPIO();

    int error();
    
    // Radio Frontend - Define GPIO settings for CM4 hat module
    uint8_t m_setGPIORX = 0x00;       // GPIO0=LOW - RX, GPIO1=LOW - PA off, GPIO2=LOW & GPIO3=LOW - 50Mhz Bandfilter
    uint8_t m_setGPIOTXDirect = 0x0F; // GPIO0=HIGH - TX, GPIO1=HIGH - PA on, GPIO2=HIGH & GPIO3=HIGH - no Bandfilter
    uint8_t m_setGPIOTX6m = 0x03;     // GPIO0=HIGH - TX, GPIO1=HIGH - PA on, GPIO2=LOW & GPIO3=LOW - 50Mhz Bandfilter
    uint8_t m_setGPIOTX2m = 0x07;     // GPIO0=HIGH - TX, GPIO1=HIGH - PA on, GPIO2=HIGH & GPIO3=LOW - 144Mhz Bandfilter
    uint8_t m_setGPIOTX70cm = 0x0B;   // GPIO0=HIGH - TX, GPIO1=HIGH - PA on, GPIO2=LOW & GPIO3=HIGH - 433Mhz Bandfilter

    std::string m_modeName[9] = {"RX", "TXDirect", "TX6m", "TX2m", "TX70cm"};
    uint8_t m_modeGPIO[9] = {m_setGPIORX, m_setGPIOTXDirect, m_setGPIOTX6m, m_setGPIOTX2m, m_setGPIOTX70cm};

};
