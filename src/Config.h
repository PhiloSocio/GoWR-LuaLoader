#pragma once

#include "Logger.h"

struct LoaderConfig {
    bool LoadScripts = true;
    std::vector<std::string> ScriptRoots = { "mod", "mods/scripts" };
    bool ScanScriptsRecursively = true;
    LogLevel ConsoleLogLevel = LogLevel::Warning;
    LogLevel FileLogLevel = LogLevel::Info;
};

bool LoadConfig(const std::filesystem::path& path, LoaderConfig& out, std::string& errorOut);
