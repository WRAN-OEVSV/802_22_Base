#pragma once

#include <memory>
#include "spdlog/spdlog.h"
#include "spdlog/sinks/stdout_color_sinks.h"
#include "spdlog/sinks/rotating_file_sink.h"

#define DEFAULT_RPX100_LOG_FILE "/var/log/RPX-100.log"

class Log {
public:

    static void Init();
    static void Init(int level);
    static void Init(int level, spdlog::filename_t logfile_name);

    static inline std::shared_ptr<spdlog::logger>& getRadioLogger() { return s_RadioLogger; }
    static inline std::shared_ptr<spdlog::logger>& getPhyLogger() { return s_PhyLogger; }
    static inline std::shared_ptr<spdlog::logger>& getTestLogger() { return s_TestLogger; }
    static inline std::shared_ptr<spdlog::logger>& getWebLogger() { return s_WebLogger; }
    static inline std::shared_ptr<spdlog::logger>& getAppLogger() {return s_AppLogger; }

private:

    static std::shared_ptr<spdlog::logger> s_RadioLogger;
    static std::shared_ptr<spdlog::logger> s_PhyLogger;
    static std::shared_ptr<spdlog::logger> s_TestLogger;
    static std::shared_ptr<spdlog::logger> s_WebLogger;
    static std::shared_ptr<spdlog::logger> s_AppLogger;
};

#define LOG_RADIO_WARN(...)    Log::getRadioLogger()->warn(__VA_ARGS__)
#define LOG_RADIO_INFO(...)    Log::getRadioLogger()->info(__VA_ARGS__)
#define LOG_RADIO_ERROR(...)   Log::getRadioLogger()->error(__VA_ARGS__)
#define LOG_RADIO_TRACE(...)   Log::getRadioLogger()->trace(__VA_ARGS__)
#define LOG_RADIO_DEBUG(...)   Log::getRadioLogger()->debug(__VA_ARGS__)

#define LOG_PHY_WARN(...)      Log::getPhyLogger()->warn(__VA_ARGS__)
#define LOG_PHY_INFO(...)      Log::getPhyLogger()->info(__VA_ARGS__)
#define LOG_PHY_ERROR(...)     Log::getPhyLogger()->error(__VA_ARGS__)
#define LOG_PHY_TRACE(...)     Log::getPhyLogger()->trace(__VA_ARGS__)
#define LOG_PHY_DEBUG(...)     Log::getPhyLogger()->debug(__VA_ARGS__)

#define LOG_TEST_WARN(...)      Log::getTestLogger()->warn(__VA_ARGS__)
#define LOG_TEST_INFO(...)      Log::getTestLogger()->info(__VA_ARGS__)
#define LOG_TEST_ERROR(...)     Log::getTestLogger()->error(__VA_ARGS__)
#define LOG_TEST_TRACE(...)     Log::getTestLogger()->trace(__VA_ARGS__)
#define LOG_TEST_DEBUG(...)     Log::getTestLogger()->debug(__VA_ARGS__)

#define LOG_WEB_WARN(...)      Log::getWebLogger()->warn(__VA_ARGS__)
#define LOG_WEB_INFO(...)      Log::getWebLogger()->info(__VA_ARGS__)
#define LOG_WEB_ERROR(...)     Log::getWebLogger()->error(__VA_ARGS__)
#define LOG_WEB_TRACE(...)     Log::getWebLogger()->trace(__VA_ARGS__)
#define LOG_WEB_DEBUG(...)     Log::getWebLogger()->debug(__VA_ARGS__)

#define LOG_APP_WARN(...)      Log::getAppLogger()->warn(__VA_ARGS__)
#define LOG_APP_INFO(...)      Log::getAppLogger()->info(__VA_ARGS__)
#define LOG_APP_ERROR(...)     Log::getAppLogger()->error(__VA_ARGS__)
#define LOG_APP_TRACE(...)     Log::getAppLogger()->trace(__VA_ARGS__)
#define LOG_APP_DEBUG(...)     Log::getAppLogger()->debug(__VA_ARGS__)
