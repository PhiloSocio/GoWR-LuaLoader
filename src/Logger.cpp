#include "Logger.h"
#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/sinks/basic_file_sink.h>

std::shared_ptr<spdlog::logger> Logger::s_Debug = nullptr;
std::shared_ptr<spdlog::logger> Logger::s_File = nullptr;
LogLevel Logger::s_DebugLevel = LogLevel::Warning;
LogLevel Logger::s_FileLevel = LogLevel::Info;

spdlog::level::level_enum Logger::ToSpd(LogLevel l) {
    switch (l) {
        case LogLevel::Trace: return spdlog::level::trace;
        case LogLevel::Debug: return spdlog::level::debug;
        case LogLevel::Info: return spdlog::level::info;
        case LogLevel::Warning: return spdlog::level::warn;
        case LogLevel::Error: return spdlog::level::err;
        case LogLevel::Critical: return spdlog::level::critical;
        default: return spdlog::level::off;
    }
}

LogLevel Logger::ParseLevel(const std::string& s) {
    if (s == "trace") return LogLevel::Trace;
    if (s == "debug") return LogLevel::Debug;
    if (s == "info") return LogLevel::Info;
    if (s == "warning" || s == "warn") return LogLevel::Warning;
    if (s == "error") return LogLevel::Error;
    if (s == "critical") return LogLevel::Critical;
    return LogLevel::Off;
}

void Logger::Initialize(const std::filesystem::path& logDir, LogLevel debugLevel, LogLevel fileLevel) {
    s_DebugLevel = debugLevel;
    s_FileLevel = fileLevel;

    if (debugLevel != LogLevel::Off) {
        try {
            s_Debug = spdlog::stdout_color_mt("gowr_console");
            s_Debug->set_level(spdlog::level::trace);
            s_Debug->set_pattern("[%H:%M:%S] [%^%l%$] %v");
            s_Debug->flush_on(spdlog::level::warn);
        } catch (...) {
            s_Debug = nullptr;
        }
    }

    if (fileLevel != LogLevel::Off) {
        try {
            std::filesystem::create_directories(logDir);
            auto logPath = (logDir / "loader_log.txt").string();
            s_File = spdlog::basic_logger_mt("gowr_file", logPath, true);
            s_File->set_level(spdlog::level::trace);
            s_File->set_pattern("[%Y-%m-%d %H:%M:%S.%e] [%l] [thread %t] %v");
            s_File->flush_on(spdlog::level::info);
        } catch (...) {
            s_File = nullptr;
        }
    }
}

void Logger::Shutdown() {
    if (s_Debug) { s_Debug->flush(); s_Debug.reset(); }
    if (s_File)    { s_File->flush();    s_File.reset(); }
    spdlog::shutdown();
}

void Logger::Log(LogLevel level, const std::string& message) {
    if (level == LogLevel::Off) return;
    auto spd = ToSpd(level);
    if (s_Debug && level >= s_DebugLevel) s_Debug->log(spd, message);
    if (s_File && level >= s_FileLevel)       s_File->log(spd, message);
}
