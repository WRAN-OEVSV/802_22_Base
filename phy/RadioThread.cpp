#include "phy/RadioThread.h"
#include "lime/LimeSuite.h"
#include "liquid/liquid.h"
#include <memory>
#include <thread>
#include <chrono>
#include <iostream>
#include <fstream> 
#include <complex>


#define SPIN_WAIT_SLEEP_MS 5

RadioThread::RadioThread() {
    LOG_RADIO_TRACE("RadioThread constructor");
    terminated.store(false);
    stopping.store(false);
}

RadioThread::~RadioThread() {
    terminated.store(true);
    stopping.store(true);
}

void RadioThread::threadMain() {
    terminated.store(false);
    stopping.store(false);
    try {
        run();
    }
    catch (...) {
        terminated.store(true);
        stopping.store(true);
        throw;
    }
  
    terminated.store(true);
    stopping.store(true);
}

void RadioThread::setup() {
    //redefined in subclasses
}

void RadioThread::run() {
    //redefined in subclasses
}


void RadioThread::terminate() {
    stopping.store(true);
}

bool RadioThread::isTerminated(int waitMs) {

    if (terminated.load()) {
        return true;
    }
    else if (waitMs == 0) {
        return false;
    }

    //this is a stupid busy plus sleep loop
    int nbCyclesToWait;

    if (waitMs < 0) {
        nbCyclesToWait = std::numeric_limits<int>::max();
    }
    else {
        nbCyclesToWait = (waitMs / SPIN_WAIT_SLEEP_MS) + 1;
    }

    for ( int i = 0; i < nbCyclesToWait; i++) {

        std::this_thread::sleep_for(std::chrono::milliseconds(SPIN_WAIT_SLEEP_MS));

        if (terminated.load()) {
            return true;
        }
    }

    std::cout << "ERROR: thread '" << typeid(*this).name() << "' has not terminated in time ! (> " << waitMs << " ms)" << std::endl << std::flush;

    return terminated.load();
}


void RadioThread::setFrequency(double frequency)
{
    // defined in radio specific class (e.g. LimeRadioThread)
}

void RadioThread::getIQData()
{
    // defined in radio specific class (e.g. LimeRadioThread)
}

void RadioThread::setIQData()
{
    // defined in radio specific class (e.g. LimeRadioThread)
}


