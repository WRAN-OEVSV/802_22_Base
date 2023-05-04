#include "phy/DefaultRadioConfig.h"
#include "LimeRadio.h"
#include "lime/LimeSuite.h"
#include "liquid/liquid.h"
#include <memory>
#include <chrono>
#include <complex>


//////////////////////////
/// LimeRadio stuff

LimeRadio::LimeRadio() 
    : Radio(), m_rxSampleCnt{Radio::m_sampleBufferCnt}, m_txSampleCnt{Radio::m_sampleBufferCnt}  {

    LOG_RADIO_TRACE("LimeRadio() constructor");

    initLimeSDR();

    initLimeGPIO();
    set_HW_RX();

    initStreaming();
}

LimeRadio::LimeRadio(int sampleBufferCnt) 
    : Radio(sampleBufferCnt), m_rxSampleCnt{Radio::m_sampleBufferCnt}, m_txSampleCnt{Radio::m_sampleBufferCnt}  {

    LOG_RADIO_TRACE("LimeRadio(int) constructor");

    initLimeSDR();

    initLimeGPIO();
    set_HW_RX();

    initStreaming();




}

LimeRadio::~LimeRadio() {

    LOG_RADIO_TRACE("LimeRadio destructor");

    stopStreaming();
    closeLimeSDR();
}


int LimeRadio::receive_IQ_data() {

    // LOG_RADIO_INFO("LimeRadio::receive_IQ_data(): receive IQ data from current SDR");

    m_isRxTxRunning.store(true);
    m_IQdataRXBuffer = Radio::getRXBuffer();

    // std::cout << m_IQdataRXBuffer << std::endl;

    // std::cout << m_IQdataRXBuffer << std::endl;
    // std::cout << m_IQdataRXBuffer->data.size() << std::endl;

//    m_IQdataTXBuffer = std::static_pointer_cast<RadioIQDataPtr>( Radio::getTXBuffer());


    // LOG_RADIO_DEBUG("run() rx stream handle {}", m_rx_streamId.handle);
    // LOG_RADIO_DEBUG("run() m_IQdataRXBuffer.use_count {}", m_IQdataRXBuffer.use_count());

    // printRadioConfig();

 
    if(m_isRX) {


        auto t1 = std::chrono::steady_clock::now();


        // clear out RX buffer from potential old stuff
        m_IQdataRXBuffer->data.clear();

        // Receive samples
        // @todo - check flag when there is a buffer overflow on the Lime .. i.e. we are getting samples too slow
        int samplesRead = LMS_RecvStream(&m_rx_streamId, m_rxIQbuffer, m_rxSampleCnt, &m_rx_metadata, 100);     // timeout 500->100

        auto t2 = std::chrono::steady_clock::now();

        LMS_GetStreamStatus(&m_rx_streamId, &m_rx_status);

        auto t21 = std::chrono::steady_clock::now();

        //I and Q samples are interleaved in buffer: IQIQIQ...
        float *pp = (float *)m_rxIQbuffer;
        liquid_float_complex s;

        m_IQdataRXBuffer->timestampFirstSample = m_rx_metadata.timestamp;

        if(samplesRead > 0)
        {

            for (int i = 0; i < samplesRead; i++) {
                s.real(pp[2 * i]);
                s.imag(pp[2 * i + 1]);

                m_IQdataRXBuffer->data.push_back(s);
            }
        } else {
            LOG_RADIO_DEBUG("no samples received!!");
        }


        auto t3 = std::chrono::steady_clock::now();

        std::cout << "ts read : ";
        std::cout << std::chrono::duration_cast<std::chrono::microseconds>(t2 - t1).count() << " : ";
        std::cout << std::chrono::duration_cast<std::chrono::microseconds>(t21 - t1).count() << " : ";
        std::cout << std::chrono::duration_cast<std::chrono::microseconds>(t3 - t2).count() << std::endl;


    } else {
        LOG_RADIO_ERROR("receive_IQ_data(): m_isRX is false -- are we set to transmit ????");
    }

    m_isRxTxRunning.store(false);

    return 0;

}


int LimeRadio::send_IQ_data() {

//    LOG_RADIO_INFO("send IQ data from current buffer.");

    m_isRxTxRunning.store(true);

    m_IQdataTXBuffer = Radio::getTXBuffer();

//    LOG_RADIO_DEBUG("run() tx stream handle {}", m_tx_streamId.handle);
//    LOG_RADIO_DEBUG("run() m_IQdataTXBuffer {}", m_IQdataTXBuffer.use_count());

//    printRadioConfig();

    if(!m_isRX) {

        auto samplesWrite = m_IQdataTXBuffer->data.size();

        if(samplesWrite > m_txSampleCnt) {
            LOG_RADIO_DEBUG("transmit buffer {} too small for {} number of samples!!", m_txSampleCnt, samplesWrite);
            return -1;
        }

        //I and Q samples are interleaved in buffer: IQIQIQ...
        float *pp = (float *)m_txIQbuffer;          // @todo check if malloc should not be done in this function (if yes it is maybe slow)
        liquid_float_complex s;

        if(samplesWrite > 0)
        {

            for (int i = 0; i < samplesWrite; i++) {

                s = m_IQdataTXBuffer->data.front();
                pp[2 * i] = s.real();
                pp[2 * i + 1] = s.imag();

                m_IQdataTXBuffer->data.pop_front();
            }

            //Send samples with delay from RX (waitForTimestamp is enabled)
            m_tx_metadata.timestamp = m_IQdataTXBuffer->timestampFirstSample;
            LMS_SendStream(&m_tx_streamId, m_txIQbuffer, samplesWrite, &m_tx_metadata, 500);    // @todo error handling on send error
        //    std::cout << m_tx_metadata.timestamp << std::endl;

            // for testing send without metadata
            // LMS_SendStream(&m_tx_streamId, m_txIQbuffer, samplesWrite, nullptr, 500);    // @todo error handling on send error

            // flush TX buffer
            m_IQdataTXBuffer->data.clear();

        } else {
            LOG_RADIO_DEBUG("no samples sent!!");
        }

    } else {
        LOG_RADIO_ERROR("send_IQ_data(): m_isRX is true -- are we set to receive ????");
    }

    m_isRxTxRunning.store(false);

    return 0;

}




int LimeRadio::send_Tone() {

    m_isRxTxRunning.store(true);

    if(!m_isRX) {

        int samplesWrite = 1280;

        if(samplesWrite > m_txSampleCnt) {
            LOG_RADIO_DEBUG("transmit buffer {} too small for {} number of samples!!", m_txSampleCnt, samplesWrite);
            return -1;
        }

        //I and Q samples are interleaved in buffer: IQIQIQ...
        float tx_buffer[2*samplesWrite];          // @todo check if malloc should not be done in this function (if yes it is maybe slow)

        const double tone_freq = 0.6e6;
        const double f_ratio = tone_freq / DEFAULT_SAMPLE_RATE;

        const double gain = 0.05;

        const double pi = acos(-1);

        for (int i = 0; i < samplesWrite; i++) {

            double w = 2*pi*i*f_ratio;

            tx_buffer[2 * i] = cos(w) * gain;
            tx_buffer[2 * i + 1] = sin(w) * gain;

        }

        //Send samples with delay from RX (waitForTimestamp is enabled)
//        m_tx_metadata.timestamp = m_IQdataTXBuffer->timestampFirstSample;
        LMS_SendStream(&m_tx_streamId, tx_buffer, samplesWrite, nullptr, 100);    // @todo error handling on send error

    } else {
        LOG_RADIO_ERROR("send_IQ_data(): m_isRX is true -- are we set to receive ????");
    }

    m_isRxTxRunning.store(false);

    return 0;

}



uint64_t LimeRadio::get_rx_timestamp() {

    // LOG_RADIO_INFO("LimeRadio::receive_IQ_data(): receive IQ data from current SDR");

    //m_isRxTxRunning.store(true);
    //m_IQdataRXBuffer = Radio::getRXBuffer();

    // std::cout << m_IQdataRXBuffer << std::endl;

    // std::cout << m_IQdataRXBuffer << std::endl;
    // std::cout << m_IQdataRXBuffer->data.size() << std::endl;

//    m_IQdataTXBuffer = std::static_pointer_cast<RadioIQDataPtr>( Radio::getTXBuffer());

 
    // clear out RX buffer from potential old stuff
    //m_IQdataRXBuffer->data.clear();

    // Receive samples to get current timestamp
    //int samplesRead = LMS_RecvStream(&m_rx_streamId, m_rxIQbuffer, 4000, &m_rx_metadata, 500);


    // @todo -- update to class member variables

    // lms_stream_status_t rx_status;
    // lms_stream_status_t tx_status;

    LMS_GetStreamStatus(&m_rx_streamId, &m_rx_status);
    LMS_GetStreamStatus(&m_tx_streamId, &m_tx_status);



    // std::cout << "RX: " << rx_status.timestamp << " : " << rx_status.overrun << " : " << rx_status.underrun << " : " << rx_status.droppedPackets << std::endl;
    // std::cout << "TX: " << tx_status.timestamp << " : " << tx_status.overrun << " : " << tx_status.underrun << " : " << tx_status.droppedPackets << std::endl;

    //m_isRxTxRunning.store(false);

    return m_rx_status.timestamp;
//    return m_rx_metadata.timestamp;

}




int LimeRadio::error()
{
    LOG_RADIO_ERROR("LimeRadio::error() called");
    if (m_lms_device != nullptr)
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
int LimeRadio::initLimeSDR() {
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
    if (LMS_Open(&m_lms_device, list[0], nullptr))
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


    // set RX Antennna, Gain and calibrate
    // LMS_SetAntenna ??
    if(LMS_SetNormalizedGain(m_lms_device, LMS_CH_RX, LMS_Channel, DEFAULT_NOM_RX_GAIN) != 0)
        error();

    LMS_Calibrate(m_lms_device, LMS_CH_RX, LMS_Channel, DEFAULT_SAMPLE_RATE, 0 );



    // set TX Antennna, Gain and calibrate
    // if(LMS_SetAntenna(m_lms_device, LMS_CH_TX, LMS_Channel, LMS_PATH_TX1) != 0)
    //     error();

    if(LMS_SetNormalizedGain(m_lms_device, LMS_CH_TX, LMS_Channel, DEFAULT_NOM_TX_GAIN) != 0)
        error();

    LMS_Calibrate(m_lms_device, LMS_CH_TX, LMS_Channel, DEFAULT_SAMPLE_RATE, 0 ); 



    return 0;
}


void LimeRadio::closeLimeSDR() {

    LOG_RADIO_INFO("close lime sdr");

    //Close device
    LMS_Close(m_lms_device);
}


void LimeRadio::initStreaming() {

    //RX Streaming Setup
    LOG_RADIO_INFO("Init RX Streaming");

    //Initialize stream
    m_rx_streamId.channel = LMS_Channel;                 // channel number
    m_rx_streamId.fifoSize = 1024 * 100;                 // fifo size in samples
    m_rx_streamId.throughputVsLatency = 1;             // throughput vs speed -- 0.5 middle - 1.0 fastest
    m_rx_streamId.isTx = false;                          // RX channel
    m_rx_streamId.dataFmt = lms_stream_t::LMS_FMT_F32;   // F32 not optimal but easier with liquid @todo check for 12bit
    if (LMS_SetupStream(m_lms_device, &m_rx_streamId) != 0)
        error();

    //allocate memory for IQ Buffer used by LMS_Receive
    if (m_rxIQbuffer != nullptr) {
            ::free(m_rxIQbuffer);
    }
    m_rxIQbuffer = std::malloc(m_rxSampleCnt * 4 * sizeof(float));

    LOG_RADIO_TRACE("initStreaming() rxSampleCnt {}", m_rxSampleCnt);    
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
    m_tx_streamId.fifoSize = 1024 * 100;                //fifo size in samples
    m_tx_streamId.throughputVsLatency = 1;             //optimize for max throughput
    m_tx_streamId.isTx = true;                          //RX channel
    m_tx_streamId.dataFmt = lms_stream_t::LMS_FMT_F32;   //F32 not optimal but easier with liquid @todo check for 12bit
    if (LMS_SetupStream(m_lms_device, &m_tx_streamId) != 0)
        error();

    //allocate memory for IQ Buffer used by LMS_Receive
    if (m_txIQbuffer != nullptr) {
            ::free(m_txIQbuffer);
    }
    m_txIQbuffer = std::malloc(m_txSampleCnt * 4 * sizeof(float));

    
    LOG_RADIO_TRACE("initStreaming() txSampleCnt {}", m_txSampleCnt);    
    LOG_RADIO_TRACE("initStreaming() m_txIQbuffer {}",m_txIQbuffer);
    LOG_RADIO_TRACE("initStreaming() size m_txIQbuffer {}", m_txSampleCnt * 4 * sizeof(int16_t));

    //Start streaming
    if(LMS_StartStream(&m_tx_streamId) != 0)
        LOG_RADIO_ERROR("TX StartStream Error");

    LOG_RADIO_TRACE("initStreaming() rx stream handle {}", m_tx_streamId.handle);

    //Streaming Metadata
    m_tx_metadata.flushPartialPacket = true; //currently has no effect in RX
    m_tx_metadata.waitForTimestamp = true; //currently has no effect in RX


}

void LimeRadio::stopStreaming() {

    //Stop streaming
    LOG_RADIO_INFO("Stop Streaming");

    std::free(m_rxIQbuffer);

    LMS_StopStream(&m_rx_streamId); //stream is stopped but can be started again with LMS_StartStream()
    LMS_DestroyStream(m_lms_device, &m_rx_streamId); //stream is deallocated and can no longer be used

    std::free(m_txIQbuffer);

    LMS_StopStream(&m_tx_streamId); //stream is stopped but can be started again with LMS_StartStream()
    LMS_DestroyStream(m_lms_device, &m_tx_streamId); //stream is deallocated and can no longer be used


}

void LimeRadio::setFrequency(float_t frequency)
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

void LimeRadio::setSamplingRate(float_t sampling_rate, size_t oversampling)
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

void LimeRadio::printRadioConfig()
{
    float_type rate, rf_rate;

    LMS_GetSampleRate(m_lms_device, LMS_CH_RX, 0, &rate, &rf_rate );
    LOG_RADIO_DEBUG("current RX host_samp_rate {} rf_samp_rate {}", rate, rf_rate);

    LMS_GetSampleRate(m_lms_device, LMS_CH_TX, 0, &rate, &rf_rate );
    LOG_RADIO_DEBUG("current TX host_samp_rate {} rf_samp_rate {}", rate, rf_rate);


}

int LimeRadio::initLimeGPIO()
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
Radio Frontend - Define GPIO settings for RPX-100 hat module
uint8_t setRX = 0x00;       // GPIO0=LOW - RX, GPIO1=LOW - PA off, GPIO2=LOW & GPIO3=LOW - 50Mhz Bandfilter
uint8_t setTXDirect = 0x0F; // GPIO0=HIGH - TX, GPIO1=HIGH - PA on, GPIO2=HIGH & GPIO3=HIGH - no Bandfilter
uint8_t setTX6m = 0x03;     // GPIO0=HIGH - TX, GPIO1=HIGH - PA on, GPIO2=LOW & GPIO3=LOW - 50Mhz Bandfilter
uint8_t setTX2m = 0x07;     // GPIO0=HIGH - TX, GPIO1=HIGH - PA on, GPIO2=HIGH & GPIO3=LOW - 144Mhz Bandfilter
uint8_t setTX70cm = 0x0B;   // GPIO0=HIGH - TX, GPIO1=HIGH - PA on, GPIO2=LOW & GPIO3=HIGH - 433Mhz Bandfilter
string modeName[5] = {"RX", "TXDirect", "TX6m", "TX2m", "TX70cm"};
uint8_t modeGPIO[5] = {setRX, setTXDirect, setTX6m, setTX2m, setTX70cm};
*/

/**
 * @brief Set GPIO to indicate to peripheral unit, that SDR is ON (e.g. LED)
 * @brief current value of gpio_val || 0x10
 *
 */
void LimeRadio::set_HW_SDR_ON() {

    LOG_RADIO_TRACE("set_HW_SDR_ON() called");

    uint8_t gpio_val = 0;
    if (LMS_GPIORead(m_lms_device, &gpio_val, 1) != 0)
    {
        error();
    }
    gpio_val = gpio_val || 0x10;
    if (LMS_GPIOWrite(m_lms_device, &gpio_val, 1) != 0)
    {
        error();
    }

    // Read and log GPIO values
    if (LMS_GPIORead(m_lms_device, &gpio_val, 1) != 0)
    {
        error();
    }

    LOG_RADIO_TRACE("set_HW_SDR_ON() gpio readback {0:x}", gpio_val);

}

/**
 * @brief Set GPIO to indicate to peripheral unit, that SDR is OFF (e.g. LED)
 * @brief current value of gpio_val || 0x00
 *
 */
void LimeRadio::set_HW_SDR_OFF() {

    LOG_RADIO_TRACE("set_HW_SDR_OFF() called");

    uint8_t gpio_val = 0;
    if (LMS_GPIORead(m_lms_device, &gpio_val, 1) != 0)
    {
        error();
    }
    gpio_val = gpio_val || 0x00;
    if (LMS_GPIOWrite(m_lms_device, &gpio_val, 1) != 0)
    {
        error();
    }

    // Read and log GPIO values
    if (LMS_GPIORead(m_lms_device, &gpio_val, 1) != 0)
    {
        error();
    }

    LOG_RADIO_TRACE("set_HW_SDR_OFF() gpio readback {0:x}", gpio_val);

}

/**
 * @brief Set GPIO for RX mode (RF switch, PA off,...)
 * @brief GPIO0=LOW - RX, GPIO1=LOW - PA off, GPIO2=LOW & GPIO3=LOW - 50Mhz Bandfilter
 * @brief Set GPIO to indicate to peripheral unit, that RX mode is set (e.g. LED)
 * @brief current value of gpio_val || 0x20
 *
 */
void LimeRadio::set_HW_RX() {

    LOG_RADIO_TRACE("set_HW_RX() called");

    m_isRX.store(true);

    // GPIO0=LOW - RX, GPIO1=LOW - PA off, GPIO2=LOW & GPIO3=LOW - 50Mhz Bandfilter
    // Set GPIOs to RX mode
    //
    uint8_t gpio_val = m_modeGPIO[0] || 0x20;
    if (LMS_GPIOWrite(m_lms_device, &gpio_val, 1) != 0)
    {
        error();
    }

    // Read and log GPIO values
    if (LMS_GPIORead(m_lms_device, &gpio_val, 1) != 0)
    {
        error();
    }

    LOG_RADIO_TRACE("set_HW_RX() gpio readback {0:x}", gpio_val);

}

/**
 * @brief set TX mode based on Radio::TxMode::<mode>
 *
 * @param m TxMode enum
 */
void LimeRadio::set_HW_TX(TxMode m) {

    // TX_DIRECT=1,
    // TX_6M,
    // TX_2M,
    // TX_70cm

    m_isRX.store(false);

    uint8_t gpio_val = m_modeGPIO[m] || 0x40;
    // Set GPIOs to TX mode
    switch (m)
    {
        case 1: // DIRECT

            LOG_RADIO_TRACE("set_HW_TX() TX_DIRECT");

            // GPIO0=HIGH - TX, GPIO1=HIGH - PA on, GPIO2=HIGH & GPIO3=HIGH - no Bandfilter

            if (LMS_GPIOWrite(m_lms_device, &gpio_val, 1) != 0)
            {
                error();
            }

            break;

        case 2: // TX_6M
            LOG_RADIO_TRACE("set_HW_TX() TX_6M");

            // GPIO0=HIGH - TX, GPIO1=HIGH - PA on, GPIO2=LOW & GPIO3=LOW - 50Mhz Bandfilter
            if (LMS_GPIOWrite(m_lms_device, &gpio_val, 1) != 0)
            {
                error();
            }

            break;
        case 3: // TX_2M
            LOG_RADIO_TRACE("set_HW_TX() TX_2M");

            // GPIO0=HIGH - TX, GPIO1=HIGH - PA on, GPIO2=HIGH & GPIO3=LOW - 144Mhz Bandfilter
            if (LMS_GPIOWrite(m_lms_device, &gpio_val, 1) != 0)
            {
                error();
            }

            break;
        case 4: // TX_70cm
            LOG_RADIO_TRACE("set_HW_TX() TX_70cm");

            // GPIO0=HIGH - TX, GPIO1=HIGH - PA on, GPIO2=LOW & GPIO3=HIGH - 433Mhz Bandfilter
            if (LMS_GPIOWrite(m_lms_device, &gpio_val, 1) != 0)
            {
                error();
            }

            break;

        default:
            LOG_RADIO_ERROR("set_HW_TX() unkown TX Mode");
            break;
    }

    // Read and log GPIO values
    if (LMS_GPIORead(m_lms_device, &gpio_val, 1) != 0)
    {
        error();
    }

    LOG_RADIO_TRACE("set_HW_TX() gpio readback {0:x}", gpio_val);
}
