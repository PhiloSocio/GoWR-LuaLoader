#include "LuaManager.h"
#include "Logger.h"
#include "CrashHandler.h"
extern "C" {
    #include <lua.h>
    #include <lualib.h>
    #include <lauxlib.h>
}
#include <mutex>

static lua_State* L = nullptr;
static std::mutex g_LuaMutex;

static int CustomLuaPrint(lua_State* L) {
    int nargs = lua_gettop(L);
    std::string out;
    for (int i = 1; i <= nargs; i++) {
        if (i > 1) out += "\t";
        if (lua_isstring(L, i)) {
            size_t len = 0;
            const char* s = lua_tolstring(L, i, &len);
            if (s) out.append(s, len);
        } else {
            out += "<";
            out += luaL_typename(L, i);
            out += ">";
        }
    }
    Logger::Log(LogLevel::Info, "[lua] " + out);
    return 0;
}

static int LuaLogInfo(lua_State* L)    { Logger::Log(LogLevel::Info,    std::string("[lua] ") + luaL_checkstring(L, 1)); return 0; }
static int LuaLogWarn(lua_State* L)    { Logger::Log(LogLevel::Warning, std::string("[lua] ") + luaL_checkstring(L, 1)); return 0; }
static int LuaLogError(lua_State* L)   { Logger::Log(LogLevel::Error,   std::string("[lua] ") + luaL_checkstring(L, 1)); return 0; }
static int LuaLogDebug(lua_State* L)   { Logger::Log(LogLevel::Debug,   std::string("[lua] ") + luaL_checkstring(L, 1)); return 0; }

void LuaManager::Initialize(const std::filesystem::path& scriptsDir) {
    std::lock_guard<std::mutex> lock(g_LuaMutex);
    CrashHandler::WriteBreadcrumb("LuaManager::Initialize:enter");

    L = luaL_newstate();
    if (!L) {
        Logger::Log(LogLevel::Error, "Failed to create Lua state");
        return;
    }

    luaL_openlibs(L);

    lua_register(L, "print", CustomLuaPrint);
    lua_register(L, "log_info",  LuaLogInfo);
    lua_register(L, "log_warn",  LuaLogWarn);
    lua_register(L, "log_error", LuaLogError);
    lua_register(L, "log_debug", LuaLogDebug);

    lua_newtable(L);
    lua_pushstring(L, "AnArchos");
    lua_setfield(L, -2, "author");
    lua_pushstring(L, "GoWR-LuaLoader");
    lua_setfield(L, -2, "name");
    lua_pushstring(L, "1.0.0");
    lua_setfield(L, -2, "version");
    lua_setglobal(L, "LOADER");

    if (!std::filesystem::exists(scriptsDir)) {
        std::filesystem::create_directories(scriptsDir);
    }

    int loaded = 0;
    int failed = 0;

    for (const auto& entry : std::filesystem::directory_iterator(scriptsDir)) {
        if (!entry.is_regular_file() || entry.path().extension() != ".lua") continue;

        std::string filePath = entry.path().string();
        CrashHandler::WriteBreadcrumb(("load:" + entry.path().filename().string()).c_str());

        if (luaL_loadfile(L, filePath.c_str()) != LUA_OK) {
            const char* err = lua_tostring(L, -1);
            Logger::Log(LogLevel::Error, "load failed: " + filePath + " - " + (err ? err : "?"));
            lua_pop(L, 1);
            failed++;
            continue;
        }
        if (lua_pcall(L, 0, LUA_MULTRET, 0) != LUA_OK) {
            const char* err = lua_tostring(L, -1);
            Logger::Log(LogLevel::Error, "exec failed: " + filePath + " - " + (err ? err : "?"));
            lua_pop(L, 1);
            failed++;
            continue;
        }
        Logger::Log(LogLevel::Info, "loaded: " + filePath);
        loaded++;
    }

    Logger::Log(
        LogLevel::Info, 
        "Lua scripts: " + std::to_string(loaded) + " loaded, " + std::to_string(failed) + " failed"
    );
    CrashHandler::WriteBreadcrumb("LuaManager::Initialize:leave");
}

void LuaManager::Shutdown() {
    std::lock_guard<std::mutex> lock(g_LuaMutex);
    if (L) {
        lua_close(L);
        L = nullptr;
        Logger::Log(LogLevel::Info, "Lua state closed");
    }
}
