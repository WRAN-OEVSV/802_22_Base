#pragma once

#include <complex>
#include <iostream>
#include <fstream>
#include <chrono>
#include <vector>
#include <deque>
#include <mutex>
#include <atomic>
#include <unistd.h>

#include "liquid/liquid.h"

#include "util/log.h"


#define IQDEBUG_BUFFER_LENGTH       350000
#define IQDEBUG_FREEZE_PERIOD       50000
#define IQDEBUG_DUMP_FILE           "IQData.txt"


// class PhyIQDebugData {



// };


class PhyIQDebug {
public:

    PhyIQDebug();
    
    ~PhyIQDebug();

    void push_iq(uint64_t ts, liquid_float_complex sample);

    void dump_iq();

    void clear_iq() { m_IQDebugData.clear(); }

    void freeze() { m_freeze.store(true); }

    uint64_t m_bufferspace_left;

private:

    std::atomic_bool m_stopped;
    std::atomic_bool m_freeze;

    std::string m_dumpfile = IQDEBUG_DUMP_FILE;

    uint64_t m_timestamp;
    uint64_t m_freeze_period;

    std::deque<liquid_float_complex> m_IQDebugData;

};


typedef std::shared_ptr<PhyIQDebug> PhyIQDebugPtr;