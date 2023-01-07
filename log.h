#include <memory>
#include "spdlog/spdlog.h"
#include "spdlog/sinks/stdout_color_sinks.h"

class Log {
public:

    static void Init();

    static inline std::shared_ptr<spdlog::logger>& getRadioLogger() { return s_RadioLogger; }

private:

    static std::shared_ptr<spdlog::logger> s_RadioLogger;

};

#define LOG_RADIO_WARN(...)    Log::getRadioLogger()->warn(__VA_ARGS__)
#define LOG_RADIO_INFO(...)    Log::getRadioLogger()->info(__VA_ARGS__)
#define LOG_RADIO_ERROR(...)   Log::getRadioLogger()->error(__VA_ARGS__)
#define LOG_RADIO_TRACE(...)   Log::getRadioLogger()->trace(__VA_ARGS__)
#define LOG_RADIO_DEBUG(...)   Log::getRadioLogger()->debug(__VA_ARGS__)
