//
// Created by fabian on 13.02.23.
//

#pragma once
#ifndef WRAN_WEBSOCKETSINK_H
#define WRAN_WEBSOCKETSINK_H

#include "spdlog/sinks/base_sink.h"

/**
 * A sink for spdlog to allow sending log messages to the frontend via websockets.
 * @tparam Mutex The mutex type to use
 */
template<typename Mutex>
class WebSocketSink : public spdlog::sinks::base_sink <Mutex>
{
public:
    /**
     * Send a log message to authenticated users in the frontend.
     * @param msg the message to send
     */
    void sink_it_(const spdlog::details::log_msg& msg) override;

    /**
     * Part of the interface - currently a no-op.
     */
    void flush_() override;
};

#include "spdlog/details/null_mutex.h"
#include <mutex>
using WebSocketSink_mt = WebSocketSink<std::mutex>;
using WebSocketSink_st = WebSocketSink<spdlog::details::null_mutex>;


#endif //WRAN_WEBSOCKETSINK_H
