#include "sovereign/core/logger.hpp"
#include <iostream>

namespace sovereign {

Logger::~Logger() {
    if (file_stream_.is_open()) {
        file_stream_.close();
    }
}

Logger& Logger::instance() {
    static Logger logger_instance;
    return logger_instance;
}

void Logger::set_level(LogLevel level) noexcept {
    std::lock_guard<std::mutex> lock(mutex_);
    level_ = level;
}

LogLevel Logger::level() const noexcept {
    return level_;
}

void Logger::set_log_file(std::string_view path) {
    std::lock_guard<std::mutex> lock(mutex_);
    if (file_stream_.is_open()) {
        file_stream_.close();
    }
    if (!path.empty()) {
        file_stream_.open(std::string(path), std::ios::out | std::ios::app);
    }
}

void Logger::log(LogLevel lvl, std::string_view msg) {
    if (lvl < level_ || level_ == LogLevel::Off) {
        return;
    }

    std::lock_guard<std::mutex> lock(mutex_);
    std::string_view prefix;
    switch (lvl) {
        case LogLevel::Debug: prefix = "[DEBUG] "; break;
        case LogLevel::Info:  prefix = "[INFO]  "; break;
        case LogLevel::Warn:  prefix = "[WARN]  "; break;
        case LogLevel::Error: prefix = "[ERROR] "; break;
        default: break;
    }

    if (file_stream_.is_open()) {
        file_stream_ << prefix << msg << "\n";
        file_stream_.flush();
    } else {
        if (lvl >= LogLevel::Warn) {
            std::cerr << prefix << msg << "\n";
        } else {
            std::cout << prefix << msg << "\n";
        }
    }
}

void Logger::debug(std::string_view msg) { log(LogLevel::Debug, msg); }
void Logger::info(std::string_view msg)  { log(LogLevel::Info, msg); }
void Logger::warn(std::string_view msg)  { log(LogLevel::Warn, msg); }
void Logger::error(std::string_view msg) { log(LogLevel::Error, msg); }

} // namespace sovereign
