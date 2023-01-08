#include "phy/RadioThread.h"
#include "RadioConfig.h"
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
    initStreaming();
}

LimeRadioThread::~LimeRadioThread() {

    LOG_RADIO_TRACE("LimeRadioThread destructor");

    stopStreaming();
    closeLimeSDR();
}

void LimeRadioThread::setFrequency(double frequency)
{
    LOG_RADIO_TRACE("LimeRadioThread set freq to {} MHZ", frequency);

    //Set center frequency to freq
    if (LMS_SetLOFrequency(m_lms_device, LMS_CH_RX, 0, frequency) != 0)
        error();
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

    IQdataOutQueue = std::static_pointer_cast<RadioThreadIQDataQueue>( RadioThread::getRXQueue());

    LOG_RADIO_DEBUG("run() stream handle {}", m_rx_streamId.handle);
    LOG_RADIO_TRACE("run() IQbuffer {}", IQbuffer);

    LOG_RADIO_TRACE("run() IQdataOutQueue {}", IQdataOutQueue.use_count());

    int sec_aquis = 2;
    double samplesTotal = 0;
 
    LOG_RADIO_TRACE("SDR thread run for {} sec",sec_aquis);

    // run for sec_aquis seconds or thread gets terminated (stopping -> true)
    while((std::chrono::high_resolution_clock::now() - t1 < std::chrono::seconds(sec_aquis)) && !stopping)
    {

        //Receive samples
        int samplesRead = LMS_RecvStream(&m_rx_streamId, IQbuffer, sampleCnt, &m_rx_metadata, 500);

        //I and Q samples are interleaved in buffer: IQIQIQ...
        float *pp = (float *)IQbuffer;
        liquid_float_complex s;


        // @tcheck  suboptimal ?? as it creates a new object for every fetch .. 
        IQdataOut = std::make_shared<RadioThreadIQData>();

        IQdataOut->timestampFirstSample = m_rx_metadata.timestamp;

        if(samplesRead > 0)
        {

            for (int i = 0; i < samplesRead; i++) {
                s.real(pp[2 * i]);
                s.imag(pp[2 * i + 1]);

                IQdataOut->data.push_back(s);
            }
        }

        // add new sample buffer block in queue
        if(!IQdataOutQueue->push(IQdataOut)) {
            LOG_RADIO_ERROR("run() queue is full - somebody to take the data - quick!!");
        }

        samplesTotal += samplesRead;

    }

    m_isReceiverRunning.store(false);
    LOG_RADIO_TRACE("Total Samples {}", samplesTotal);
    LOG_RADIO_INFO("SDR thread done.");

}

void LimeRadioThread::terminate() {
    RadioThread::terminate();

    IQdataOutQueue = std::static_pointer_cast<RadioThreadIQDataQueue>( RadioThread::getRXQueue());

    if (IQdataOutQueue != nullptr) {
        IQdataOutQueue->flush();
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

    //Set center frequency 
    if (LMS_SetLOFrequency(m_lms_device, LMS_CH_RX, LMS_Channel, DEFAULT_CENTER_FREQ) != 0)
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

    //Streaming Setup
    LOG_RADIO_INFO("Init Streaming");

    //Initialize stream
    m_rx_streamId.channel = LMS_Channel;                 //channel number
    m_rx_streamId.fifoSize = 1024 * 1024;                //fifo size in samples
    m_rx_streamId.throughputVsLatency = 1.0;             //optimize for max throughput
    m_rx_streamId.isTx = false;                          //RX channel
    m_rx_streamId.dataFmt = lms_stream_t::LMS_FMT_F32;   //F32 not optimal but easier with liquid @todo check for 12bit
    if (LMS_SetupStream(m_lms_device, &m_rx_streamId) != 0)
        error();

    //allocate memory for IQ Buffer used by LMS_Receive
    if (IQbuffer != nullptr) {
            ::free(IQbuffer);
    }
    IQbuffer = std::malloc(sampleCnt * 4 * sizeof(float));

    
    LOG_RADIO_TRACE("initStreaming() IQbuffer {}",IQbuffer);
    LOG_RADIO_TRACE("initStreaming() size IQbuffer {}", sampleCnt * 4 * sizeof(int16_t));

    //Start streaming
    if(LMS_StartStream(&m_rx_streamId) != 0)
        LOG_RADIO_ERROR("StartStream Error");

    LOG_RADIO_TRACE("initStreaming() stream handle {}", m_rx_streamId.handle);

    //Streaming Metadata
    m_rx_metadata.flushPartialPacket = false; //currently has no effect in RX
    m_rx_metadata.waitForTimestamp = false; //currently has no effect in RX


}

void LimeRadioThread::stopStreaming() {

    //Stop streaming
    LOG_RADIO_INFO("Stop Streaming");

    std::free(IQbuffer);

    LMS_StopStream(&m_rx_streamId); //stream is stopped but can be started again with LMS_StartStream()
    LMS_DestroyStream(m_lms_device, &m_rx_streamId); //stream is deallocated and can no longer be used

}
