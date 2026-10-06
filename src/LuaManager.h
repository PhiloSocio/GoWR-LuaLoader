#pragma once

#include <filesystem>

namespace LuaManager {
    void Initialize(const std::filesystem::path& scriptsDir);
    void Shutdown();
}
