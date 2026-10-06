#pragma once

#include <filesystem>

namespace LuaManager {
    void Initialize(const std::vector<std::filesystem::path>& scriptRoots, bool recursive);
    void Shutdown();
}
