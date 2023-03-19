#include "log.h"
#include "spdlog/sinks/base_sink.h"
#include "spdlog/sinks/rotating_file_sink.h"
#include <mutex>
#include "WebSocketServer.h"

std::shared_ptr<spdlog::logger> Log::s_RadioLogger;
std::shared_ptr<spdlog::logger> Log::s_PhyLogger;
std::shared_ptr<spdlog::logger> Log::s_TestLogger;
std::shared_ptr<spdlog::logger> Log::s_AppLogger;

/**
 * A sink for spdlog to allow sending log messages to the frontend via websockets.
 * @tparam Mutex The mutex type to use
 */
template<typename Mutex>
class WebSocketSink : public spdlog::sinks::base_sink <Mutex>
{
protected:
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

using WebSocketSink_mt = WebSocketSink<std::mutex>;

template<typename Mutex>
void WebSocketSink<Mutex>::flush_() {
    // do nothing;
}


template<typename Mutex>
void WebSocketSink<Mutex>::sink_it_(const spdlog::details::log_msg &msg) {
    spdlog::memory_buf_t formatted;
    spdlog::sinks::base_sink<Mutex>::formatter_->format(msg, formatted);
    if (webSocketServer != nullptr) {
        webSocketServer->broadcast_log(fmt::to_string(formatted));
    }
}


void Log::Init() {

    spdlog::set_pattern("%^[%T] %n: %v%$");
    auto console_sink = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
    auto socket_sink = std::make_shared<WebSocketSink_mt>();
    auto max_size = 1048576 * 5;
    auto max_files = 3;
    auto file_sink = std::make_shared<spdlog::sinks::rotating_file_sink_mt>("/var/log/RPX-100.log", max_size, max_files, false);

    s_RadioLogger = std::make_shared<spdlog::logger>(spdlog::logger("RADIO", {console_sink, socket_sink }));
    s_RadioLogger->set_level(spdlog::level::trace);

    s_PhyLogger = std::make_shared<spdlog::logger>(spdlog::logger("PHY", {console_sink, socket_sink }));;
    s_PhyLogger->set_level(spdlog::level::trace);

    s_TestLogger = std::make_shared<spdlog::logger>(spdlog::logger("TEST", {console_sink, socket_sink }));;
    s_TestLogger->set_level(spdlog::level::trace);

    s_AppLogger = std::make_shared<spdlog::logger>(spdlog::logger("APP", {file_sink, socket_sink }));;
    s_AppLogger->set_level(spdlog::level::trace);


}
