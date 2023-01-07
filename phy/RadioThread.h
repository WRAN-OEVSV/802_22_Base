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
#include "RadioConfig.h"
#include "log.h"

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

    }

    RadioThreadIQData(long long bandwidth, long long frequency, std::vector<signed char> * /* data */) :
            frequency(frequency), sampleRate(bandwidth), timestampFirstSample(0) {

    }

    virtual ~RadioThreadIQData() = default;
};

typedef std::shared_ptr<RadioThreadIQData> RadioThreadIQDataPtr;


/**
 * RadioThreadIQDataQueue class
 *
 * @note for storing IQ sample blocks - currently more a quick hack to get IQ data to verify it
 *
 */
class RadioThreadIQDataQueue {
public:

    std::vector<RadioThreadIQDataPtr> iq_queue;

    RadioThreadIQDataPtr current;
 
    RadioThreadIQDataQueue() { 

        LOG_RADIO_DEBUG("RadioThreadIQDataQueue() constructor");
        // add new object to iq_queue

        current = std::make_shared<RadioThreadIQData>();
        iq_queue.push_back(current);

    }

    void add() {

        current = std::make_shared<RadioThreadIQData>();
        iq_queue.push_back(current);
    }


    ~RadioThreadIQDataQueue() { 
        LOG_RADIO_DEBUG("RadioThreadIQDataQueue() de-constructor");
    }
};


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

    //Returns true if the thread is indeed terminated, i.e the run() method
    //has returned. 
    //If wait > 0 ms, the call is blocking at most 'waitMs' milliseconds for the thread to die, then returns.
    //If wait < 0, the wait in infinite until the thread dies.
    bool isTerminated(int waitMs = 0);
    

    // SDR Radio stuff as virutal functions which are then defined in the specifc radio class

    /**
     * Set RF center frequency in Hz.
     *
     * @note tbd
     *
     * @param   frequency   Desired RF center frequency in Hz
     *
     * @return  void
     */
    virtual void setFrequency(double frequency);

    /**
     * get IQ Data from Queue
     *
     * @note currently used for debug - is writing a CSV file with the buffer content
     *
     * @param   none
     *
     * @return  void
     */
    virtual void getIQData();
    virtual void setIQData();

    
protected:
    std::atomic_bool stopping;

private:
    //true when the thread has really ended, i.e run() from threadMain() has returned.
    std::atomic_bool terminated;

};

