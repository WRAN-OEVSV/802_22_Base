#include "PhyIQDebug.h"


PhyIQDebug::PhyIQDebug() {

    LOG_PHY_INFO("PhyIQDebug::PhyIQDebug() constructor");
    m_bufferspace_left = IQDEBUG_BUFFER_LENGTH;
    
    m_stopped.store(false);
    m_freeze.store(false);

    m_timestamp = 0;
    m_freeze_period = IQDEBUG_FREEZE_PERIOD;

}

PhyIQDebug::~PhyIQDebug() {

    LOG_PHY_INFO("PhyIQDebug::~PhyIQDebug() de-constructor");
    m_stopped.store(true);
    m_freeze.store(true);

    dump_iq();      // dumps the data on de-construction - just in case it was not dumped before

    m_IQDebugData.clear(); // cleanup

}


void PhyIQDebug::push_iq(uint64_t ts, liquid_float_complex sample) {

    if(!m_stopped) {

        m_timestamp = ts;
        m_IQDebugData.push_back(sample);

        if(m_IQDebugData.size() > IQDEBUG_BUFFER_LENGTH) {
            // size of queue has reached max .. making room in the front
            m_IQDebugData.pop_front();
        }


        // if the debug gets freezed the iq data collection is running for the freeze period and then stops
        if(m_freeze)
            m_freeze_period--;

        if(!m_freeze_period)
            m_stopped.store(true);

        m_bufferspace_left = m_IQDebugData.size();
    }


}

/**
 * @brief dumps the IQ data from the deque buffer to a file
 * 
 */
void PhyIQDebug::dump_iq() {

    // write data for debug to ramdisk (cmdline -w option) - anyways  is even with ram very slow ...
    // root@x:~# mkdir /tmp/ramdisk
    // root@x:~# chmod 777 ramdisk/
    // root@x:~# mount -t tmpfs -o size=200M myramdisk /tmp/ramdisk/

    // bool stop = true;

    // if(result.count("w")) {
    //     write_csv_file(sdr, iqpipe_rx,"/tmp/ramdisk/IQData.csv");
    //     stop = false;
    // }

    uint64_t t = 0;

    std::ofstream myfile(IQDEBUG_DUMP_FILE);

    // error checking if file is there ... maybe a bit overkill for a test script :-)
    if(!myfile.is_open()) {
        LOG_TEST_ERROR("cannot open {}", IQDEBUG_DUMP_FILE);

    } else {

        // get the received IQ data via the queue which is attached to the SDR thread

        t = m_timestamp - m_IQDebugData.size();

        for(auto s: m_IQDebugData)
        {
            if(myfile.is_open())
                myfile << s.real() << ", " << s.imag()<< ", " << t << std::endl;
            t++;
        }

        myfile.close();
        LOG_TEST_INFO("iq data dumpe to {} done", IQDEBUG_DUMP_FILE);
    }


}