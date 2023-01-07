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
    if (LMS_SetLOFrequency(device, LMS_CH_RX, 0, frequency) != 0)
        error();
}

void LimeRadioThread::getIQData()
{
    LOG_RADIO_TRACE("LimeRadioThread get IQ data");

    std::ofstream myfile("IQData.csv");


    std::cout << IQdataOutQueue.iq_queue.size() << std::endl;

    for(auto b: IQdataOutQueue.iq_queue)
    {

        uint64_t t = b->timestampFirstSample;

        std::cout << "timestamp " << t  << std::endl;

        for(auto s: b->data)
        {
            myfile << s.real() << ", " << s.imag()<< ", " << t << std::endl;
            t++;
        }

    }

    myfile.close();

}

void LimeRadioThread::setIQData()
{
    LOG_RADIO_TRACE("LimeRadioThread set IQ data");

}



void LimeRadioThread::run() {


    LOG_RADIO_INFO("SDR thread starting.");

    auto t1 = std::chrono::high_resolution_clock::now();

    LOG_RADIO_DEBUG("run() stream handle {}", streamId.handle);
    LOG_RADIO_TRACE("run() IQbuffer {}", IQbuffer);


    int sec_aquis = 2;
    double samplesTotal = 0;
 
    LOG_RADIO_TRACE("SDR thread run for {} sec",sec_aquis);
    while(std::chrono::high_resolution_clock::now() - t1 < std::chrono::seconds(sec_aquis))
    {

        //Receive samples
        int samplesRead = LMS_RecvStream(&streamId, IQbuffer, sampleCnt, &rx_metadata, 1000);

 	
        //I and Q samples are interleaved in buffer: IQIQIQ...


        float *pp = (float *)IQbuffer;
        liquid_float_complex s;

//        std::cout << "timestamp of samplebatch: " << rx_metadata.timestamp << std::endl;

        IQdataOutQueue.current.get()->timestampFirstSample = rx_metadata.timestamp;

        if(samplesRead > 0)
        {

            for (int i = 0; i < samplesRead; i++) {
                s.real(pp[2 * i]);
                s.imag(pp[2 * i + 1]);

                IQdataOutQueue.current.get()->data.push_back(s);
            }
        }

        // add new sample buffer block in queue
        IQdataOutQueue.add();

        samplesTotal += samplesRead;

    }

    LOG_RADIO_TRACE("Total Samples {}", samplesTotal);
    LOG_RADIO_INFO("SDR thread done.");

}

void LimeRadioThread::terminate() {
    RadioThread::terminate();

}

int LimeRadioThread::error()
{
    LOG_RADIO_ERROR("LimeRadioThread::error() called");
    if (device != NULL)
        LMS_Close(device);
    exit(-1);
}




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
    if (LMS_Open(&device, list[0], NULL))
        error();

    //Initialize device with default configuration
    //Do not use if you want to keep existing configuration
    //Use LMS_LoadConfig(device, "/path/to/file.ini") to load config from INI
    if (LMS_Init(device) != 0)
        error();


    //Enable RX channel
    //Channels are numbered starting at 0
    if (LMS_EnableChannel(device, LMS_CH_RX, LMS_Channel, true) != 0)
        error();

    //Set center frequency to 50 MHz
    if (LMS_SetLOFrequency(device, LMS_CH_RX, LMS_Channel, DEFAULT_CENTER_FREQ) != 0)
        error();

    //This set sampling rate for all channels
    if (LMS_SetSampleRate(device, DEFAULT_SAMPLE_RATE, DEFAULT_OVERSAMPLING) != 0)
        error();

    return 0;
}


void LimeRadioThread::closeLimeSDR() {

    LOG_RADIO_INFO("close lime sdr");

    //Close device
    LMS_Close(device);
}


void LimeRadioThread::initStreaming() {

    //Streaming Setup
    LOG_RADIO_INFO("Init Streaming");

    //Initialize stream
    streamId.channel = LMS_Channel;                 //channel number
    streamId.fifoSize = 1024 * 1024;                //fifo size in samples
    streamId.throughputVsLatency = 1.0;             //optimize for max throughput
    streamId.isTx = false;                          //RX channel
    streamId.dataFmt = lms_stream_t::LMS_FMT_F32;   //F32 not optimal but easier with liquid @todo check for 12bit
    if (LMS_SetupStream(device, &streamId) != 0)
        error();

    //allocate memory for IQ Buffer used by LMS_Receive
    if (IQbuffer != nullptr) {
            ::free(IQbuffer);
    }
    IQbuffer = std::malloc(sampleCnt * 4 * sizeof(float));

    
    LOG_RADIO_TRACE("initStreaming() IQbuffer {}",IQbuffer);
    LOG_RADIO_TRACE("initStreaming() size IQbuffer {}", sampleCnt * 4 * sizeof(int16_t));

    //Start streaming
    if(LMS_StartStream(&streamId) != 0)
        LOG_RADIO_ERROR("StartStream Error");

    LOG_RADIO_TRACE("initStreaming() stream handle {}", streamId.handle);

    //Streaming Metadata
    rx_metadata.flushPartialPacket = false; //currently has no effect in RX
    rx_metadata.waitForTimestamp = false; //currently has no effect in RX


}

void LimeRadioThread::stopStreaming() {

    //Stop streaming
    LOG_RADIO_INFO("Stop Streaming");

    std::free(IQbuffer);

    LMS_StopStream(&streamId); //stream is stopped but can be started again with LMS_StartStream()
    LMS_DestroyStream(device, &streamId); //stream is deallocated and can no longer be used

}
