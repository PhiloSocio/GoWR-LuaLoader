#include "Logger.h"
#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/sinks/basic_file_sink.h>

std::shared_ptr<spdlog::logger> Logger::s_Console = nullptr;
std::shared_ptr<spdlog::logger> Logger::s_File = nullptr;
LogLevel Logger::s_ConsoleLevel = LogLevel::Warning;
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

void Logger::Initialize(const std::filesystem::path& logDir, LogLevel consoleLevel, LogLevel fileLevel) {
    s_ConsoleLevel = consoleLevel;
    s_FileLevel = fileLevel;

    if (consoleLevel != LogLevel::Off) {
        try {
            s_Console = spdlog::stdout_color_mt("gowr_console");
            s_Console->set_level(spdlog::level::trace);
            s_Console->set_pattern("[%H:%M:%S] [%^%l%$] %v");
            s_Console->flush_on(spdlog::level::warn);
        } catch (...) {
            s_Console = nullptr;
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
    if (s_Console) { s_Console->flush(); s_Console.reset(); }
    if (s_File)    { s_File->flush();    s_File.reset(); }
    spdlog::shutdown();
}

void Logger::Log(LogLevel level, const std::string& message) {
    if (level == LogLevel::Off) return;
    auto spd = ToSpd(level);
    if (s_Console && level >= s_ConsoleLevel) s_Console->log(spd, message);
    if (s_File && level >= s_FileLevel)       s_File->log(spd, message);
}
