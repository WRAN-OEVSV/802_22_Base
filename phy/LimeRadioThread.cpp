#include "phy/RadioThread.h"
#include "phy/DefaultRadioConfig.h"
#include "LimeRadioThread.h"
#include "lime/LimeSuite.h"
#include "liquid/liquid.h"
#include <memory>
#include <thread>
#include <chrono>
#include <iostream>
#include <fstream> 
#include <complex>


//////////////////////////
/// LimeRadioThread stuff

LimeRadioThread::LimeRadioThread() : RadioThread() {

    LOG_RADIO_TRACE("LimeRadioThread constructor");

    initLimeSDR();

    initLimeGPIO();
    set_HW_RX();

    initStreaming();
}

LimeRadioThread::~LimeRadioThread() {

    LOG_RADIO_TRACE("LimeRadioThread destructor");

    stopStreaming();
    closeLimeSDR();
}



/**
 * @brief run() is (currently only) getting the data from the Lime - a quick hack for the start
 * 
 * @todo move receive stuff to own function and add RX/TX case; thread either runs as RX or TX
 * 
 */
void LimeRadioThread::run() {


    LOG_RADIO_INFO("SDR thread starting.");

    m_isReceiverRunning.store(true);

    auto t1 = std::chrono::high_resolution_clock::now();

    m_IQdataRXQueue = std::static_pointer_cast<RadioThreadIQDataQueue>( RadioThread::getRXQueue());

    LOG_RADIO_DEBUG("run() stream handle {}", m_rx_streamId.handle);
    LOG_RADIO_TRACE("run() m_rxIQbuffer {}", m_rxIQbuffer);

    LOG_RADIO_TRACE("run() m_IQdataRXQueue {}", m_IQdataRXQueue.use_count());

    printRadioConfig();

    int sec_aquis = 2;
    double samplesTotalRX = 0;
 
    LOG_RADIO_TRACE("SDR thread run for {} sec",sec_aquis);

    // run for sec_aquis seconds or thread gets terminated (stopping -> true)
    while((std::chrono::high_resolution_clock::now() - t1 < std::chrono::seconds(sec_aquis)) && !stopping)
    {

        if(m_isRX) {
            // SDR is running in RX mode

            //Receive samples
            int samplesRead = LMS_RecvStream(&m_rx_streamId, m_rxIQbuffer, m_rxSampleCnt, &m_rx_metadata, 500);

            //I and Q samples are interleaved in buffer: IQIQIQ...
            float *pp = (float *)m_rxIQbuffer;
            liquid_float_complex s;


            // @tcheck  suboptimal ?? as it creates a new object for every fetch .. 
            m_rxIQdataOut = std::make_shared<RadioThreadIQData>();

            m_rxIQdataOut->timestampFirstSample = m_rx_metadata.timestamp;

            if(samplesRead > 0)
            {

                for (int i = 0; i < samplesRead; i++) {
                    s.real(pp[2 * i]);
                    s.imag(pp[2 * i + 1]);

                    m_rxIQdataOut->data.push_back(s);
                }
            }

            // add new sample buffer block in queue
            if(!m_IQdataRXQueue->push(m_rxIQdataOut)) {
                LOG_RADIO_ERROR("run() queue is full - somebody to take the data - quick!!");
            }

            samplesTotalRX += samplesRead;

        } else {
            // sdr running in TX mode
        }


    }

    m_isReceiverRunning.store(false);
    LOG_RADIO_TRACE("Total Samples RX {}", samplesTotalRX);
    LOG_RADIO_INFO("SDR thread done.");

}

void LimeRadioThread::terminate() {
    RadioThread::terminate();

    m_IQdataRXQueue = std::static_pointer_cast<RadioThreadIQDataQueue>( RadioThread::getRXQueue());

    if (m_IQdataRXQueue != nullptr) {
        m_IQdataRXQueue->flush();
    }

    m_IQdataTXQueue = std::static_pointer_cast<RadioThreadIQDataQueue>( RadioThread::getTXQueue());

    if (m_IQdataTXQueue != nullptr) {
        m_IQdataTXQueue->flush();
    }
}

int LimeRadioThread::error()
{
    LOG_RADIO_ERROR("LimeRadioThread::error() called");
    if (m_lms_device != NULL)
        LMS_Close(m_lms_device);
    exit(-1);
}



/**
 * @brief initalize the LimeSDR to default values
 * 
 * @todo currently only RX expand to TX, and add better error handling if no devices are found
 * 
 * @return int 
 */
int LimeRadioThread::initLimeSDR() {
    // init LimeSDR
    LOG_RADIO_INFO("Init limesdr");

    //Find devices
    int n;
    lms_info_str_t list[8]; //should be large enough to hold all detected devices
    if ((n = LMS_GetDeviceList(list)) < 0) //NULL can be passed to only get number of devices
        error();

    LOG_RADIO_TRACE("Devices found: {}", n); //print number of devices
    if (n < 1)
        return -1;

    //open the first device
    if (LMS_Open(&m_lms_device, list[0], NULL))
        error();

    //Initialize device with default configuration
    //Do not use if you want to keep existing configuration
    //Use LMS_LoadConfig(device, "/path/to/file.ini") to load config from INI
    if (LMS_Init(m_lms_device) != 0)
        error();


    //Enable RX channel
    //Channels are numbered starting at 0
    if (LMS_EnableChannel(m_lms_device, LMS_CH_RX, LMS_Channel, true) != 0)
        error();

    //Set RX center frequency 
    if (LMS_SetLOFrequency(m_lms_device, LMS_CH_RX, LMS_Channel, DEFAULT_CENTER_FREQ) != 0)
        error();

    //Enable TX channel
    //Channels are numbered starting at 0
    if (LMS_EnableChannel(m_lms_device, LMS_CH_TX, LMS_Channel, true) != 0)
        error();

    //Set TX center frequency 
    if (LMS_SetLOFrequency(m_lms_device, LMS_CH_TX, LMS_Channel, DEFAULT_CENTER_FREQ) != 0)
        error();


    //This set sampling rate for all channels
    if (LMS_SetSampleRate(m_lms_device, DEFAULT_SAMPLE_RATE, DEFAULT_OVERSAMPLING) != 0)
        error();

    return 0;
}


void LimeRadioThread::closeLimeSDR() {

    LOG_RADIO_INFO("close lime sdr");

    //Close device
    LMS_Close(m_lms_device);
}


void LimeRadioThread::initStreaming() {

    //RX Streaming Setup
    LOG_RADIO_INFO("Init RX Streaming");

    //Initialize stream
    m_rx_streamId.channel = LMS_Channel;                 //channel number
    m_rx_streamId.fifoSize = 1024 * 1024;                //fifo size in samples
    m_rx_streamId.throughputVsLatency = 0.5;             //optimize for max throughput
    m_rx_streamId.isTx = false;                          //RX channel
    m_rx_streamId.dataFmt = lms_stream_t::LMS_FMT_F32;   //F32 not optimal but easier with liquid @todo check for 12bit
    if (LMS_SetupStream(m_lms_device, &m_rx_streamId) != 0)
        error();

    //allocate memory for IQ Buffer used by LMS_Receive
    if (m_rxIQbuffer != nullptr) {
            ::free(m_rxIQbuffer);
    }
    m_rxIQbuffer = std::malloc(m_rxSampleCnt * 4 * sizeof(float));

    
    LOG_RADIO_TRACE("initStreaming() m_rxIQbuffer {}",m_rxIQbuffer);
    LOG_RADIO_TRACE("initStreaming() size m_rxIQbuffer {}", m_rxSampleCnt * 4 * sizeof(int16_t));

    //Start streaming
    if(LMS_StartStream(&m_rx_streamId) != 0)
        LOG_RADIO_ERROR("RX StartStream Error");

    LOG_RADIO_TRACE("initStreaming() rx stream handle {}", m_rx_streamId.handle);

    //Streaming Metadata
    m_rx_metadata.flushPartialPacket = false; //currently has no effect in RX
    m_rx_metadata.waitForTimestamp = false; //currently has no effect in RX


    //TX Streaming Setup
    LOG_RADIO_INFO("Init TX Streaming");

    //Initialize TX stream
    m_tx_streamId.channel = LMS_Channel;                 //channel number
    m_tx_streamId.fifoSize = 1024 * 1024;                //fifo size in samples
    m_tx_streamId.throughputVsLatency = 0.5;             //optimize for max throughput
    m_tx_streamId.isTx = true;                          //RX channel
    m_tx_streamId.dataFmt = lms_stream_t::LMS_FMT_F32;   //F32 not optimal but easier with liquid @todo check for 12bit
    if (LMS_SetupStream(m_lms_device, &m_tx_streamId) != 0)
        error();

    //allocate memory for IQ Buffer used by LMS_Receive
    if (m_txIQbuffer != nullptr) {
            ::free(m_txIQbuffer);
    }
    m_txIQbuffer = std::malloc(m_txSampleCnt * 4 * sizeof(float));

    
    LOG_RADIO_TRACE("initStreaming() m_txIQbuffer {}",m_txIQbuffer);
    LOG_RADIO_TRACE("initStreaming() size m_txIQbuffer {}", m_txSampleCnt * 4 * sizeof(int16_t));

    //Start streaming
    if(LMS_StartStream(&m_tx_streamId) != 0)
        LOG_RADIO_ERROR("TX StartStream Error");

    LOG_RADIO_TRACE("initStreaming() rx stream handle {}", m_tx_streamId.handle);

    //Streaming Metadata
    m_tx_metadata.flushPartialPacket = false; //currently has no effect in RX
    m_tx_metadata.waitForTimestamp = true; //currently has no effect in RX


}

void LimeRadioThread::stopStreaming() {

    //Stop streaming
    LOG_RADIO_INFO("Stop Streaming");

    std::free(m_rxIQbuffer);

    LMS_StopStream(&m_rx_streamId); //stream is stopped but can be started again with LMS_StartStream()
    LMS_DestroyStream(m_lms_device, &m_rx_streamId); //stream is deallocated and can no longer be used

    std::free(m_txIQbuffer);

    LMS_StopStream(&m_tx_streamId); //stream is stopped but can be started again with LMS_StartStream()
    LMS_DestroyStream(m_lms_device, &m_tx_streamId); //stream is deallocated and can no longer be used


}

void LimeRadioThread::setFrequency(float_t frequency)
{
    LOG_RADIO_TRACE("setFrequency() set freq to {} MHZ", frequency);

    LMS_StopStream(&m_rx_streamId);
    LMS_StopStream(&m_tx_streamId);


    //Set center frequency to freq
    if (LMS_SetLOFrequency(m_lms_device, LMS_CH_RX, 0, frequency) != 0)
        error();

    //Set center frequency to freq
    if (LMS_SetLOFrequency(m_lms_device, LMS_CH_TX, 0, frequency) != 0)
        error();

    if(LMS_StartStream(&m_rx_streamId) != 0)
        LOG_RADIO_ERROR("RX StartStream Error");
    if(LMS_StartStream(&m_tx_streamId) != 0)
        LOG_RADIO_ERROR("TX StartStream Error");


}

void LimeRadioThread::setSamplingRate(float_t sampling_rate, size_t oversampling)
{
    LOG_RADIO_TRACE("setFrequency() set sampling_rate {} and oversampling {}", sampling_rate, oversampling);

    LMS_StopStream(&m_rx_streamId);
    LMS_StopStream(&m_tx_streamId);

    if (LMS_SetSampleRate(m_lms_device, sampling_rate, oversampling) != 0)
        error();

    if(LMS_StartStream(&m_rx_streamId) != 0)
        LOG_RADIO_ERROR("RX StartStream Error");
    if(LMS_StartStream(&m_tx_streamId) != 0)
        LOG_RADIO_ERROR("TX StartStream Error");
}

void LimeRadioThread::printRadioConfig()
{
    float_type rate, rf_rate;

    LMS_GetSampleRate(m_lms_device, LMS_CH_RX, 0, &rate, &rf_rate );
    LOG_RADIO_DEBUG("current RX host_samp_rate {} rf_samp_rate {}", rate, rf_rate);

    LMS_GetSampleRate(m_lms_device, LMS_CH_TX, 0, &rate, &rf_rate );
    LOG_RADIO_DEBUG("current TX host_samp_rate {} rf_samp_rate {}", rate, rf_rate);


}

int LimeRadioThread::initLimeGPIO()
{
    LOG_RADIO_INFO("initLimeGPIO() - setup GPIO pins");

    // Set SDR GPIO diretion GPIO0-5 to output and GPIO6-7 to input
    uint8_t gpio_dir = 0xFF;
    if (LMS_GPIODirWrite(m_lms_device, &gpio_dir, 1) != 0)
    {
        error();
    }

    // Read and log GPIO direction settings
    uint8_t gpio_val = 0;
    if (LMS_GPIODirRead(m_lms_device, &gpio_val, 1) != 0)
    {
        error();
    }


    LOG_RADIO_DEBUG("initLimeGPIO() direction {0:x}", gpio_val);

    return 0;
}

/*
Radio Frontend - Define GPIO settings for CM4 hat module
uint8_t setRX = 0x00;       // GPIO0=LOW - RX, GPIO1=LOW - PA off, GPIO2=LOW & GPIO3=LOW - 50Mhz Bandfilter
uint8_t setTXDirect = 0x0F; // GPIO0=HIGH - TX, GPIO1=HIGH - PA on, GPIO2=HIGH & GPIO3=HIGH - no Bandfilter
uint8_t setTX6m = 0x03;     // GPIO0=HIGH - TX, GPIO1=HIGH - PA on, GPIO2=LOW & GPIO3=LOW - 50Mhz Bandfilter
uint8_t setTX2m = 0x07;     // GPIO0=HIGH - TX, GPIO1=HIGH - PA on, GPIO2=HIGH & GPIO3=LOW - 144Mhz Bandfilter
uint8_t setTX70cm = 0x0B;   // GPIO0=HIGH - TX, GPIO1=HIGH - PA on, GPIO2=LOW & GPIO3=HIGH - 433Mhz Bandfilter

string modeName[9] = {"RX", "TXDirect", "TX6m", "TX2m", "TX70cm"};
uint8_t modeGPIO[9] = {setRX, setTXDirect, setTX6m, setTX2m, setTX70cm};
*/

/**
 * @brief Set GPIO for RX mode (RF switch, PA off,...)
 * @brief GPIO0=LOW - RX, GPIO1=LOW - PA off, GPIO2=LOW & GPIO3=LOW - 50Mhz Bandfilter
 * 
 */
void LimeRadioThread::set_HW_RX() {

    LOG_RADIO_DEBUG("set_HW_RX() called");

    // GPIO0=LOW - RX, GPIO1=LOW - PA off, GPIO2=LOW & GPIO3=LOW - 50Mhz Bandfilter
    // Set GPIOs to RX mode
    if (LMS_GPIOWrite(m_lms_device, &m_modeGPIO[0], 1) != 0)
    {
        error();
    }

    uint8_t gpio_val = 0;
    // Read and log GPIO values
    if (LMS_GPIORead(m_lms_device, &gpio_val, 1) != 0)
    {
        error();
    }

    LOG_RADIO_DEBUG("set_HW_RX() gpio readback {0:x}", gpio_val);

};

/**
 * @brief Set GPIO for TX direct no bandfilter
 * @brief GPIO0=HIGH - TX, GPIO1=HIGH - PA on, GPIO2=HIGH & GPIO3=HIGH - no Bandfilter
 */
void LimeRadioThread::set_HW_TX_direct() {

    LOG_RADIO_DEBUG("set_HW_TX_direct() called");
    
    // GPIO0=HIGH - TX, GPIO1=HIGH - PA on, GPIO2=HIGH & GPIO3=HIGH - no Bandfilter
    // Set GPIOs to TX mode
    if (LMS_GPIOWrite(m_lms_device, &m_modeGPIO[1], 1) != 0)
    {
        error();
    }

    uint8_t gpio_val = 0;
    // Read and log GPIO values
    if (LMS_GPIORead(m_lms_device, &gpio_val, 1) != 0)
    {
        error();
    }

    LOG_RADIO_DEBUG("set_HW_TX_direct() gpio readback {0:x}", gpio_val);
};

void LimeRadioThread::set_HW_TX_6m() {

    LOG_RADIO_DEBUG("set_HW_TX_6m() called");
    
    // GPIO0=HIGH - TX, GPIO1=HIGH - PA on, GPIO2=LOW & GPIO3=LOW - 50Mhz Bandfilter
    // Set GPIOs to TX mode
    if (LMS_GPIOWrite(m_lms_device, &m_modeGPIO[2], 1) != 0)
    {
        error();
    }

    uint8_t gpio_val = 0;
    // Read and log GPIO values
    if (LMS_GPIORead(m_lms_device, &gpio_val, 1) != 0)
    {
        error();
    }

    LOG_RADIO_DEBUG("set_HW_TX_6m() gpio readback {0:x}", gpio_val);
};

void LimeRadioThread::set_HW_TX_2m() {

    LOG_RADIO_DEBUG("set_HW_TX_2m() called");
    
    // GPIO0=HIGH - TX, GPIO1=HIGH - PA on, GPIO2=HIGH & GPIO3=LOW - 144Mhz Bandfilter
    // Set GPIOs to TX mode
    if (LMS_GPIOWrite(m_lms_device, &m_modeGPIO[3], 1) != 0)
    {
        error();
    }

    uint8_t gpio_val = 0;
    // Read and log GPIO values
    if (LMS_GPIORead(m_lms_device, &gpio_val, 1) != 0)
    {
        error();
    }

    LOG_RADIO_DEBUG("set_HW_TX_2m() gpio readback {0:x}", gpio_val);
};

void LimeRadioThread::set_HW_TX_70cm() {

    LOG_RADIO_DEBUG("set_HW_TX_70cm() called");
    
    // GPIO0=HIGH - TX, GPIO1=HIGH - PA on, GPIO2=LOW & GPIO3=HIGH - 433Mhz Bandfilter
    // Set GPIOs to TX mode
    if (LMS_GPIOWrite(m_lms_device, &m_modeGPIO[4], 1) != 0)
    {
        error();
    }

    uint8_t gpio_val = 0;
    // Read and log GPIO values
    if (LMS_GPIORead(m_lms_device, &gpio_val, 1) != 0)
    {
        error();
    }

    LOG_RADIO_DEBUG("set_HW_TX_70cm() gpio readback {0:x}", gpio_val);
};