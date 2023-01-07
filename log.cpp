#include "log.h"


std::shared_ptr<spdlog::logger> Log::s_RadioLogger;


void Log::Init() {

    spdlog::set_pattern("%^[%T] %n: %v%$");

    s_RadioLogger = spdlog::stdout_color_mt("Radio");
    s_RadioLogger->set_level(spdlog::level::trace);

}
