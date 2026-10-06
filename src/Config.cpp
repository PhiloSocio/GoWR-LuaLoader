#include "Config.h"
#include <toml.hpp>

bool LoadConfig(const std::filesystem::path& path, LoaderConfig& out, std::string& errorOut) {
    if (!std::filesystem::exists(path)) {
        errorOut = "config file not found: " + path.string();
        return false;
    }
    try {
        auto data = toml::parse(path.string());
        if (data.contains("Lua")) {
            const auto& g = toml::find(data, "Lua");
            if (g.contains("LoadScripts")) out.LoadScripts = toml::find<bool>(g, "LoadScripts");
        }
        if (data.contains("Logging")) {
            const auto& g = toml::find(data, "Logging");
            if (g.contains("ConsoleLogLevel"))  out.ConsoleLogLevel   = Logger::ParseLevel(toml::find<std::string>(g, "ConsoleLogLevel"));
            if (g.contains("FileLogLevel"))     out.FileLogLevel      = Logger::ParseLevel(toml::find<std::string>(g, "FileLogLevel"));
        }
        return true;
    } catch (const toml::syntax_error& e) {
        errorOut = std::string("toml syntax error: ") + e.what();
        return false;
    } catch (const std::exception& e) {
        errorOut = std::string("config error: ") + e.what();
        return false;
    }
}
