#pragma once

#include "Logger.h"

struct LoaderConfig {
    bool LoadScripts = true;
    LogLevel ConsoleLogLevel = LogLevel::Warning;
    LogLevel FileLogLevel = LogLevel::Info;
};

bool LoadConfig(const std::filesystem::path& path, LoaderConfig& out, std::string& errorOut);
