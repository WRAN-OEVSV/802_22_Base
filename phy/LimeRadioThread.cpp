#include "phy/RadioThread.h"
#include "phy/DefaultRadioConfig.h"
#include "LimeRadioThread.h"
#include "lime/LimeSuite.h"
#include "liquid/liquid.h"
#include <memory>
#include <chrono>
#include <complex>

//////////////////////////
/// LimeRadioThread stuff

LimeRadioThread::LimeRadioThread()
        : RadioThread(), m_rxSampleCnt{RadioThread::m_sampleBufferCnt}, m_txSampleCnt{RadioThread::m_sampleBufferCnt}
{

    LOG_RADIO_TRACE("LimeRadioThread() constructor");

    initLimeSDR();

    initLimeGPIO();
    set_HW_RX();
    set_HW_SDR_ON();

    initStreaming();
}

LimeRadioThread::LimeRadioThread(int sampleBufferCnt)
        : RadioThread(sampleBufferCnt), m_rxSampleCnt{RadioThread::m_sampleBufferCnt}, m_txSampleCnt{RadioThread::m_sampleBufferCnt}
{

    LOG_RADIO_TRACE("LimeRadioThread(int) constructor");

    initLimeSDR();

    initLimeGPIO();
    set_HW_RX();
    set_HW_SDR_ON();

    initStreaming();
}

LimeRadioThread::~LimeRadioThread()
{

    LOG_RADIO_TRACE("LimeRadioThread destructor");

    stopStreaming();
    closeLimeSDR();
}

/**
 * @brief run() is receiving and sending the data via the Lime - a quick hack for the start
 *
 * @todo move RX/TX stuff to own function ; thread either runs as RX or TX
 *
 */
void LimeRadioThread::run()
{

    LOG_RADIO_TRACE("SDR thread starting.");

    m_isRxTxRunning.store(true);

    m_IQdataRXQueue = std::static_pointer_cast<RadioThreadIQDataQueue>(RadioThread::getRXQueue());
    m_IQdataTXQueue = std::static_pointer_cast<RadioThreadIQDataQueue>(RadioThread::getTXQueue());

    LOG_RADIO_DEBUG("run() rx stream handle {}", m_rx_streamId.handle);
    LOG_RADIO_DEBUG("run() m_rxIQbuffer {}", m_rxIQbuffer);
    LOG_RADIO_DEBUG("run() m_IQdataRXQueue {}", m_IQdataRXQueue.use_count());
    LOG_RADIO_DEBUG("run() tx stream handle {}", m_tx_streamId.handle);
    LOG_RADIO_DEBUG("run() m_txIQbuffer {}", m_txIQbuffer);
    LOG_RADIO_DEBUG("run() m_IQdataTXQueue {}", m_IQdataTXQueue.use_count());

    printRadioConfig();

    double samplesTotalRX = 0;
    double samplesTotalTX = 0;

    // run until thread gets terminated, or stopped (stopping -> true)
    while (!stopping)
    {
        if (m_isRX)
        {

            // Receive samples
            int samplesRead = LMS_RecvStream(&m_rx_streamId, m_rxIQbuffer, m_rxSampleCnt, &m_rx_metadata, 500);

            // I and Q samples are interleaved in buffer: IQIQIQ...
            float *pp = (float *)m_rxIQbuffer;
            liquid_float_complex s;

            // @tcheck  suboptimal ??
            m_rxIQdataOut = std::make_shared<RadioThreadIQData>();

            m_rxIQdataOut->timestampFirstSample = m_rx_metadata.timestamp;

            if (samplesRead > 0)
            {

                for (int i = 0; i < samplesRead; i++)
                {
                    s.real(pp[2 * i]);
                    s.imag(pp[2 * i + 1]);

                    m_rxIQdataOut->data.push_back(s);
                }
            }

            // add new sample buffer block in queue
            if (!m_IQdataRXQueue->push(m_rxIQdataOut))
            {
                LOG_RADIO_ERROR("IQ buffer could not be pushed to Queue");
            }

            samplesTotalRX += samplesRead;
        }
        else
        {

            // Transmit Samples

            // @tcheck  suboptimal ??
            m_txIQdataOut = std::make_shared<RadioThreadIQData>();

            // get queue item
            if (m_IQdataTXQueue->pop(m_txIQdataOut))
            {

                auto samplesWrite = m_txIQdataOut->data.size();

                // I and Q samples are interleaved in buffer: IQIQIQ...
                float *pp = (float *)m_txIQbuffer;
                liquid_float_complex s;

                if (samplesWrite > 0)
                {

                    for (int i = 0; i < samplesWrite; i++)
                    {

                        s = m_rxIQdataOut->data.front();
                        pp[2 * i] = s.real();
                        pp[2 * i + 1] = s.imag();

                        m_rxIQdataOut->data.pop_front();
                    }

                    // Send samples with delay from RX (waitForTimestamp is enabled)
                    m_tx_metadata.timestamp = m_txIQdataOut->timestampFirstSample;
                    LMS_SendStream(&m_tx_streamId, m_txIQbuffer, samplesWrite, &m_tx_metadata, 500);

                    samplesTotalTX += samplesWrite;
                }
            }
        }
    }

    m_isRxTxRunning.store(false);
    LOG_RADIO_DEBUG("Total Samples RX {}", samplesTotalRX);
    LOG_RADIO_DEBUG("Total Samples TX {}", samplesTotalTX);
    LOG_RADIO_TRACE("SDR thread done.");
}

void LimeRadioThread::terminate()
{
    RadioThread::terminate();

    m_IQdataRXQueue = std::static_pointer_cast<RadioThreadIQDataQueue>(RadioThread::getRXQueue());

    if (m_IQdataRXQueue != nullptr)
    {
        m_IQdataRXQueue->flush();
    }

    m_IQdataTXQueue = std::static_pointer_cast<RadioThreadIQDataQueue>(RadioThread::getTXQueue());

    if (m_IQdataTXQueue != nullptr)
    {
        m_IQdataTXQueue->flush();
    }

    // GPIO STREAM LED off
    // GPIO SDR LED off
    set_HW_STREAM_LED_OFF();
    set_HW_SDR_OFF();
}

int LimeRadioThread::error()
{
    LOG_RADIO_ERROR("LimeRadioThread::error() called");
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
int LimeRadioThread::initLimeSDR()
{
    // init LimeSDR
    LOG_RADIO_TRACE("Init limesdr");

    // Find devices
    int n;
    lms_info_str_t list[8];                // should be large enough to hold all detected devices
    if ((n = LMS_GetDeviceList(list)) < 0) // NULL can be passed to only get number of devices
        error();

    LOG_RADIO_TRACE("Devices found: {}", n); // print number of devices
    if (n < 1)
        return -1;

    // open the first device
    if (LMS_Open(&m_lms_device, list[0], nullptr))
        error();

    // Initialize device with default configuration
    // Do not use if you want to keep existing configuration
    // Use LMS_LoadConfig(device, "/path/to/file.ini") to load config from INI
    if (LMS_Init(m_lms_device) != 0)
        error();

    // Enable RX channel
    // Channels are numbered starting at 0
    if (LMS_EnableChannel(m_lms_device, LMS_CH_RX, LMS_Channel, true) != 0)
        error();

    // Set RX center frequency
    if (LMS_SetLOFrequency(m_lms_device, LMS_CH_RX, LMS_Channel, DEFAULT_CENTER_FREQ) != 0)
        error();

    // Enable TX channel
    // Channels are numbered starting at 0
    if (LMS_EnableChannel(m_lms_device, LMS_CH_TX, LMS_Channel, true) != 0)
        error();

    // Set TX center frequency
    if (LMS_SetLOFrequency(m_lms_device, LMS_CH_TX, LMS_Channel, DEFAULT_CENTER_FREQ) != 0)
        error();

    // This set sampling rate for all channels
    if (LMS_SetSampleRate(m_lms_device, DEFAULT_SAMPLE_RATE, DEFAULT_OVERSAMPLING) != 0)
        error();

    LOG_APP_INFO("Started LimeSDR with SampleRate: {}, OverSampling: {}, CenterFreq: {}", DEFAULT_SAMPLE_RATE, DEFAULT_OVERSAMPLING, DEFAULT_CENTER_FREQ);

    return 0;
}

void LimeRadioThread::closeLimeSDR()
{

    LOG_RADIO_TRACE("close lime sdr");

    // Close device
    LMS_Close(m_lms_device);
}

void LimeRadioThread::initStreaming()
{

    // RX Streaming Setup
    LOG_RADIO_TRACE("Init RX Streaming");

    // Initialize stream
    m_rx_streamId.channel = LMS_Channel;               // channel number
    m_rx_streamId.fifoSize = 1024 * 1024;              // fifo size in samples
    m_rx_streamId.throughputVsLatency = 0.5;           // optimize for max throughput
    m_rx_streamId.isTx = false;                        // RX channel
    m_rx_streamId.dataFmt = lms_stream_t::LMS_FMT_F32; // F32 not optimal but easier with liquid @todo check for 12bit
    if (LMS_SetupStream(m_lms_device, &m_rx_streamId) != 0)
        error();

    // allocate memory for IQ Buffer used by LMS_Receive
    if (m_rxIQbuffer != nullptr)
    {
        ::free(m_rxIQbuffer);
    }
    m_rxIQbuffer = std::malloc(m_rxSampleCnt * 4 * sizeof(float));

    LOG_RADIO_TRACE("initStreaming() rxSampleCnt {}", m_rxSampleCnt);
    LOG_RADIO_TRACE("initStreaming() m_rxIQbuffer {}", m_rxIQbuffer);
    LOG_RADIO_TRACE("initStreaming() size m_rxIQbuffer {}", m_rxSampleCnt * 4 * sizeof(int16_t));

    // Start streaming
    if (LMS_StartStream(&m_rx_streamId) != 0)
        LOG_RADIO_ERROR("RX StartStream Error");

    LOG_RADIO_TRACE("initStreaming() rx stream handle {}", m_rx_streamId.handle);
    LOG_APP_INFO("Started RX Streaming, SampleCount: {}", m_rxSampleCnt);

    // Streaming Metadata
    m_rx_metadata.flushPartialPacket = false; // currently has no effect in RX
    m_rx_metadata.waitForTimestamp = false;   // currently has no effect in RX

    // TX Streaming Setup
    LOG_RADIO_INFO("Init TX Streaming");

    // Initialize TX stream
    m_tx_streamId.channel = LMS_Channel;               // channel number
    m_tx_streamId.fifoSize = 1024 * 1024;              // fifo size in samples
    m_tx_streamId.throughputVsLatency = 0.5;           // optimize for max throughput
    m_tx_streamId.isTx = true;                         // RX channel
    m_tx_streamId.dataFmt = lms_stream_t::LMS_FMT_F32; // F32 not optimal but easier with liquid @todo check for 12bit
    if (LMS_SetupStream(m_lms_device, &m_tx_streamId) != 0)
        error();

    // allocate memory for IQ Buffer used by LMS_Receive
    if (m_txIQbuffer != nullptr)
    {
        ::free(m_txIQbuffer);
    }
    m_txIQbuffer = std::malloc(m_txSampleCnt * 4 * sizeof(float));

    LOG_RADIO_TRACE("initStreaming() txSampleCnt {}", m_txSampleCnt);
    LOG_RADIO_TRACE("initStreaming() m_txIQbuffer {}", m_txIQbuffer);
    LOG_RADIO_TRACE("initStreaming() size m_txIQbuffer {}", m_txSampleCnt * 4 * sizeof(int16_t));

    // Start streaming
    if (LMS_StartStream(&m_tx_streamId) != 0)
        LOG_RADIO_ERROR("TX StartStream Error");

    LOG_RADIO_TRACE("initStreaming() rx stream handle {}", m_tx_streamId.handle);
    LOG_APP_INFO("Started TX Streaming, SampleCount: {}", m_txSampleCnt);

    // Streaming Metadata
    m_tx_metadata.flushPartialPacket = false; // currently has no effect in RX
    m_tx_metadata.waitForTimestamp = true;    // currently has no effect in RX

    // GPIO STREAM LED on
    set_HW_STREAM_LED_ON();
}

void LimeRadioThread::stopStreaming()
{

    // Stop streaming
    LOG_RADIO_TRACE("Stop Streaming");

    std::free(m_rxIQbuffer);

    LMS_StopStream(&m_rx_streamId);                  // stream is stopped but can be started again with LMS_StartStream()
    LMS_DestroyStream(m_lms_device, &m_rx_streamId); // stream is deallocated and can no longer be used

    std::free(m_txIQbuffer);

    LMS_StopStream(&m_tx_streamId);                  // stream is stopped but can be started again with LMS_StartStream()
    LMS_DestroyStream(m_lms_device, &m_tx_streamId); // stream is deallocated and can no longer be used

    // GPIO STREAM LED off
    set_HW_STREAM_LED_OFF();
}

void LimeRadioThread::setFrequency(float_t frequency)
{
    LOG_RADIO_TRACE("setFrequency() set freq to {} MHZ", frequency);

    LMS_StopStream(&m_rx_streamId);
    LMS_StopStream(&m_tx_streamId);

    // Set center frequency to freq
    if (LMS_SetLOFrequency(m_lms_device, LMS_CH_RX, 0, frequency) != 0)
        error();

    // Set center frequency to freq
    if (LMS_SetLOFrequency(m_lms_device, LMS_CH_TX, 0, frequency) != 0)
        error();

    if (LMS_StartStream(&m_rx_streamId) != 0)
        LOG_RADIO_ERROR("RX StartStream Error");
    if (LMS_StartStream(&m_tx_streamId) != 0)
        LOG_RADIO_ERROR("TX StartStream Error");

    LOG_APP_INFO("Set CenterFreq: {} MHz", frequency);
}

void LimeRadioThread::setSamplingRate(float_t sampling_rate, size_t oversampling)
{
    LOG_RADIO_TRACE("setFrequency() set sampling_rate {} and oversampling {}", sampling_rate, oversampling);

    LMS_StopStream(&m_rx_streamId);
    LMS_StopStream(&m_tx_streamId);

    if (LMS_SetSampleRate(m_lms_device, sampling_rate, oversampling) != 0)
        error();

    if (LMS_StartStream(&m_rx_streamId) != 0)
        LOG_RADIO_ERROR("RX StartStream Error");
    if (LMS_StartStream(&m_tx_streamId) != 0)
        LOG_RADIO_ERROR("TX StartStream Error");

    LOG_APP_INFO("Set SampleRate: {} MHz and OverSampling: {}", sampling_rate, oversampling);
}

void LimeRadioThread::printRadioConfig()
{
    float_type rate, rf_rate;

    LMS_GetSampleRate(m_lms_device, LMS_CH_RX, 0, &rate, &rf_rate);
    LOG_RADIO_DEBUG("current RX host_samp_rate {} rf_samp_rate {}", rate, rf_rate);

    LMS_GetSampleRate(m_lms_device, LMS_CH_TX, 0, &rate, &rf_rate);
    LOG_RADIO_DEBUG("current TX host_samp_rate {} rf_samp_rate {}", rate, rf_rate);
}

int LimeRadioThread::initLimeGPIO()
{
    LOG_RADIO_TRACE("initLimeGPIO() - setup GPIO pins");
    LOG_APP_INFO("Setup GPIO pins");

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

    LOG_RADIO_DEBUG("initLimeGPIO() direction {0:X}", gpio_val);
    LOG_APP_INFO("GPIO direction {0:X}", gpio_val);

    return 0;
}

/**
 * @brief Set GPIO to indicate to peripheral unit, that SDR is ON (e.g. LED)
 * @brief current value of gpio_val || 0x01
 *
 */
void LimeRadioThread::set_HW_SDR_ON()
{

    LOG_RADIO_TRACE("set_HW_SDR_ON() called");

    uint8_t gpio_val = 0;
    if (LMS_GPIORead(m_lms_device, &gpio_val, 1) != 0)
    {
        error();
    }
    gpio_val = gpio_val | m_setGPIOLED1;
    if (LMS_GPIOWrite(m_lms_device, &gpio_val, 1) != 0)
    {
        error();
    }

    // Read and log GPIO values
    if (LMS_GPIORead(m_lms_device, &gpio_val, 1) != 0)
    {
        error();
    }

    LOG_RADIO_TRACE("set_HW_SDR_ON() gpio readback {0:X}", gpio_val);
    LOG_APP_INFO("Set SDR_ON LED: {0:X}", gpio_val);
}

/**
 * @brief Set GPIO to indicate to peripheral unit, that SDR is OFF (e.g. LED)
 * @brief current value of gpio_val || 0x00
 *
 */
void LimeRadioThread::set_HW_SDR_OFF()
{

    LOG_RADIO_TRACE("set_HW_SDR_OFF() called");

    uint8_t gpio_val = 0;
    if (LMS_GPIORead(m_lms_device, &gpio_val, 1) != 0)
    {
        error();
    }
    gpio_val = gpio_val & ~m_setGPIOLED1;
    if (LMS_GPIOWrite(m_lms_device, &gpio_val, 1) != 0)
    {
        error();
    }

    // Read and log GPIO values
    if (LMS_GPIORead(m_lms_device, &gpio_val, 1) != 0)
    {
        error();
    }

    LOG_RADIO_TRACE("set_HW_SDR_OFF() gpio readback {0:X}", gpio_val);
    LOG_APP_INFO("Set SDR_OFF LED: {0:X}", gpio_val);
}

/**
 * @brief Set GPIO for RX mode (RF switch, PA off,...)
 * @brief GPIO0=HIGH - RX, GPIO1=LOW - PA off, GPIO2=LOW & GPIO3=LOW - 50Mhz Bandfilter
 * @brief Set GPIO to indicate to peripheral unit, that RX mode is set (e.g. LED)
 * @brief current value of gpio_val || 0x20
 *
 */
void LimeRadioThread::set_HW_RX()
{

    LOG_RADIO_TRACE("set_HW_RX() called");

    // GPIO0=LOW - RX, GPIO1=LOW - PA off, GPIO2=LOW & GPIO3=LOW - 50Mhz Bandfilter
    // Set GPIOs to RX mode
    //
    uint8_t gpio_val = 0;
    // Read and log GPIO values
    if (LMS_GPIORead(m_lms_device, &gpio_val, 1) != 0)
    {
        error();
    }
    gpio_val = gpio_val & m_setGPIOLED4;  // keep state of STREAM LED

    gpio_val = gpio_val | m_setGPIORX;
    gpio_val = gpio_val | m_setGPIOLED1;  // SDR LED on
    gpio_val = gpio_val | m_setGPIOLED3;  // RX LED on
    if (LMS_GPIOWrite(m_lms_device, &gpio_val, 1) != 0)
    {
        error();
    }

    // Read and log GPIO values
    if (LMS_GPIORead(m_lms_device, &gpio_val, 1) != 0)
    {
        error();
    }

    LOG_RADIO_TRACE("set_HW_RX() gpio readback {0:X}", gpio_val);
    LOG_APP_INFO("Set RX_MODE: {0:X}", gpio_val);
}

/**
 * @brief set TX mode based on RadioThread::TxMode::<mode>
 *
 * @param m TxMode enum
 */
void LimeRadioThread::set_HW_TX(uint8_t m)
{

    // TX_DIRECT=1,
    // TX_6M,
    // TX_2M,
    // TX_70cm
    uint8_t gpio_val = 0;
    // Read and log GPIO values
    if (LMS_GPIORead(m_lms_device, &gpio_val, 1) != 0)
    {
        error();
    }
    gpio_val = gpio_val & m_setGPIOLED4;  // keep state of STREAM LED

    LOG_RADIO_TRACE("set_HW_TX() {}", m_modeName[m]);
    gpio_val = gpio_val | m_modeGPIO[m];
    gpio_val = gpio_val | m_setGPIOLED2; // TX LED on
    gpio_val = gpio_val | m_setGPIOLED1; // SDR ON LED on


    if (LMS_GPIOWrite(m_lms_device, &gpio_val, 1) != 0)
    {
        error();
    }

    // Read and log GPIO values
    if (LMS_GPIORead(m_lms_device, &gpio_val, 1) != 0)
    {
        error();
    }

    LOG_RADIO_TRACE("set_HW_TX() gpio readback {0:X}", gpio_val);
    LOG_APP_INFO("Set Mode to {}", m_modeName[m]);
}

/**
 * @brief Set GPIO to indicate to peripheral unit, that SDR is ON (e.g. LED)
 * @brief current value of gpio_val || 0x01
 *
 */
void LimeRadioThread::set_HW_STREAM_LED_ON()
{

    LOG_RADIO_TRACE("set_HW_STREAM_LED_ON() called");

    uint8_t gpio_val = 0;
    if (LMS_GPIORead(m_lms_device, &gpio_val, 1) != 0)
    {
        error();
    }
    gpio_val = gpio_val | m_setGPIOLED4;
    if (LMS_GPIOWrite(m_lms_device, &gpio_val, 1) != 0)
    {
        error();
    }

    // Read and log GPIO values
    if (LMS_GPIORead(m_lms_device, &gpio_val, 1) != 0)
    {
        error();
    }

    LOG_RADIO_TRACE("set_HW_STREAM_LED_ON() gpio readback {0:X}", gpio_val);
    LOG_APP_INFO("Set STREAM LED ON: {0:X}", gpio_val);
}

void LimeRadioThread::set_HW_STREAM_LED_OFF()
{

    LOG_RADIO_TRACE("set_HW_STREAM_LED_OFF() called");

    uint8_t gpio_val = 0;
    if (LMS_GPIORead(m_lms_device, &gpio_val, 1) != 0)
    {
        error();
    }
    gpio_val = gpio_val & ~m_setGPIOLED4;
    if (LMS_GPIOWrite(m_lms_device, &gpio_val, 1) != 0)
    {
        error();
    }

    // Read and log GPIO values
    if (LMS_GPIORead(m_lms_device, &gpio_val, 1) != 0)
    {
        error();
    }

    LOG_RADIO_TRACE("set_HW_STREAM_LED_OFF() gpio readback {0:X}", gpio_val);
    LOG_APP_INFO("Set STREAM LED OFF: {0:X}", gpio_val);
}