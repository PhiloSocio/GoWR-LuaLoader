#pragma once

#include <filesystem>
#include <spdlog/spdlog.h>

enum class LogLevel {
    Off = 0,
    Trace = 1,
    Debug = 2,
    Info = 3,
    Warning = 4,
    Error = 5,
    Critical = 6
};

class Logger {
public:
    static void Initialize(const std::filesystem::path& logDir, LogLevel debugLevel, LogLevel fileLevel);
    static void Shutdown();
    static void Log(LogLevel level, const std::string& message);
    static LogLevel ParseLevel(const std::string& s);
    static spdlog::level::level_enum ToSpd(LogLevel l);
private:
    static std::shared_ptr<spdlog::logger> s_Debug;
    static std::shared_ptr<spdlog::logger> s_File;
    static LogLevel s_DebugLevel;
    static LogLevel s_FileLevel;
};
