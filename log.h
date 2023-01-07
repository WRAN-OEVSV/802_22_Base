#include <memory>
#include "spdlog/spdlog.h"
#include "spdlog/sinks/stdout_color_sinks.h"

class Log {
public:

    static void Init();

    static inline std::shared_ptr<spdlog::logger>& getRadioLogger() { return s_RadioLogger; }
    static inline std::shared_ptr<spdlog::logger>& getPhyLogger() { return s_PhyLogger; }

private:

    static std::shared_ptr<spdlog::logger> s_RadioLogger;
    static std::shared_ptr<spdlog::logger> s_PhyLogger;

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
