#pragma once

#include <unistd.h> // usleep
#include <string.h>
#include <stdio.h>
#include <math.h>
#include <complex.h>
#include <complex>
#include <iostream>
#include <fstream>

#include "liquid.h"

#include "phy/RadioThread.h"
#include "util/log.h"
#include "util/WebSocketServer.h"


#define SOCKET_TIMEOUT 50


class wsSpectrogram : public WebSocketServer {
public:


    wsSpectrogram(int port);

    ~wsSpectrogram();


    bool isWsRunning() {
        return m_isWsRunning.load();
    }

    //the thread Main call back itself
    void threadMain();

    void run();

    //Request for termination (asynchronous)
    void terminate();


    void setQueue(const ThreadIQDataQueueBasePtr& threadQueue);
    ThreadIQDataQueueBasePtr getQueue();

    // Overridden by children
    void onConnect(    int socketID                        );
    void onMessage(    int socketID, const string& data    );
    void onDisconnect( int socketID                        );
    void onError(      int socketID, const string& message );


protected:

    RadioThreadIQDataQueuePtr m_IQdataQueue;
    ThreadIQDataQueueBasePtr m_IQdataQueueBase;

    RadioThreadIQDataPtr m_IQdataOut;

    std::mutex m_queue_bindings_mutex;

    std::atomic_bool stopping;

    std::atomic_bool m_isWsRunning;

private:


    std::mutex m_onSockets_mutex;

    std::atomic_bool terminated;

    int m_ConCurSocket;
    std::atomic_bool m_socketsOn;

    double m_rxFreq = 52000000;
    double m_txFreq = 52000000;
    double m_span = 2285000;

    std::stringstream m_msgSOCKET;


};