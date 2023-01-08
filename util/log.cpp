#include "log.h"


std::shared_ptr<spdlog::logger> Log::s_RadioLogger;
std::shared_ptr<spdlog::logger> Log::s_PhyLogger;
std::shared_ptr<spdlog::logger> Log::s_TestLogger;

void Log::Init() {

    spdlog::set_pattern("%^[%T] %n: %v%$");

    s_RadioLogger = spdlog::stdout_color_mt("RADIO");
    s_RadioLogger->set_level(spdlog::level::trace);

    s_PhyLogger = spdlog::stdout_color_mt("PHY");
    s_PhyLogger->set_level(spdlog::level::trace);

    s_TestLogger = spdlog::stdout_color_mt("TEST");
    s_TestLogger->set_level(spdlog::level::trace);


}
