#ifndef SOVEREIGN_CORE_LOGGER_HPP
#define SOVEREIGN_CORE_LOGGER_HPP

#include <string_view>
#include <fstream>
#include <mutex>

namespace sovereign {

enum class LogLevel {
    Debug = 0,
    Info  = 1,
    Warn  = 2,
    Error = 3,
    Off   = 4
};

class Logger {
private:
    LogLevel level_{LogLevel::Info};
    std::ofstream file_stream_;
    std::mutex mutex_;

    Logger() = default;

public:
    ~Logger();
    Logger(const Logger&) = delete;
    Logger& operator=(const Logger&) = delete;

    static Logger& instance();

    void set_level(LogLevel level) noexcept;
    LogLevel level() const noexcept;
    
    void set_log_file(std::string_view path);

    void log(LogLevel lvl, std::string_view msg);
    void debug(std::string_view msg);
    void info(std::string_view msg);
    void warn(std::string_view msg);
    void error(std::string_view msg);
};

} // namespace sovereign

#endif // SOVEREIGN_CORE_LOGGER_HPP
