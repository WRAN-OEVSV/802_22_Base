//
// Created by fabian on 13.02.23.
//

#include "WebSocketSink.h"
#include "WebSocketServer.h"

template<typename Mutex>
void WebSocketSink<Mutex>::flush_() {
    // do nothing;
}

template<typename Mutex>
void WebSocketSink<Mutex>::sink_it_(const spdlog::details::log_msg &msg) {
    spdlog::memory_buf_t formatted;
    spdlog::sinks::base_sink<Mutex>::formatter_->format(msg, formatted);
    webSocketServer->broadcast_log(fmt::to_string(formatted));
}