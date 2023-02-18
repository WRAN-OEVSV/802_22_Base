#include "log.h"
#include "WebSocketSink.h"


std::shared_ptr<spdlog::logger> Log::s_RadioLogger;
std::shared_ptr<spdlog::logger> Log::s_PhyLogger;
std::shared_ptr<spdlog::logger> Log::s_TestLogger;

void Log::Init() {

    spdlog::set_pattern("%^[%T] %n: %v%$");
    auto console_sink = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
    //auto socket_sink = std::make_shared<WebSocketSink_mt>();

    s_RadioLogger = std::make_shared<spdlog::logger>(spdlog::logger("RADIO", {console_sink }));
    s_RadioLogger->set_level(spdlog::level::trace);

    s_PhyLogger = std::make_shared<spdlog::logger>(spdlog::logger("PHY", {console_sink }));;
    s_PhyLogger->set_level(spdlog::level::trace);

    s_TestLogger = std::make_shared<spdlog::logger>(spdlog::logger("TEST", {console_sink }));;
    s_TestLogger->set_level(spdlog::level::trace);


}
