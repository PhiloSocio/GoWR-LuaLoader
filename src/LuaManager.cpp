#include "LuaManager.h"
#include "Logger.h"
#include "CrashHandler.h"
extern "C" {
    #include <lua.h>
    #include <lualib.h>
    #include <lauxlib.h>
}
#include <mutex>

static lua_State* g_LuaState = nullptr;
static std::mutex g_LuaMutex;

static int LuaPrint(lua_State* g_LuaState) {
    int nargs = lua_gettop(g_LuaState);
    std::string out;
    for (int i = 1; i <= nargs; i++) {
        if (i > 1) out += "\t";
        if (lua_isstring(g_LuaState, i)) {
            size_t len = 0;
            const char* s = lua_tolstring(g_LuaState, i, &len);
            if (s) out.append(s, len);
        } else {
            out += "<";
            out += luaL_typename(g_LuaState, i);
            out += ">";
        }
    }
    Logger::Log(LogLevel::Info, "[lua] " + out);
    return 0;
}

static int LuaLogInfo(lua_State* g_LuaState)    { Logger::Log(LogLevel::Info,    std::string("[lua] ") + luaL_checkstring(g_LuaState, 1)); return 0; }
static int LuaLogWarn(lua_State* g_LuaState)    { Logger::Log(LogLevel::Warning, std::string("[lua] ") + luaL_checkstring(g_LuaState, 1)); return 0; }
static int LuaLogError(lua_State* g_LuaState)   { Logger::Log(LogLevel::Error,   std::string("[lua] ") + luaL_checkstring(g_LuaState, 1)); return 0; }
static int LuaLogDebug(lua_State* g_LuaState)   { Logger::Log(LogLevel::Debug,   std::string("[lua] ") + luaL_checkstring(g_LuaState, 1)); return 0; }

void LuaManager::Initialize(const std::filesystem::path& scriptsDir) {
    std::lock_guard<std::mutex> lock(g_LuaMutex);
    CrashHandler::WriteBreadcrumb("LuaManager::Initialize:enter");

    g_LuaState = luaL_newstate();
    if (!g_LuaState) {
        Logger::Log(LogLevel::Error, "Failed to create Lua state");
        return;
    }

    luaL_openlibs(g_LuaState);

    lua_register(g_LuaState, "print", LuaPrint);
    lua_register(g_LuaState, "log_info",  LuaLogInfo);
    lua_register(g_LuaState, "log_warn",  LuaLogWarn);
    lua_register(g_LuaState, "log_error", LuaLogError);
    lua_register(g_LuaState, "log_debug", LuaLogDebug);

    lua_newtable(g_LuaState);
    lua_pushstring(g_LuaState, LOADER_NAME);
    lua_setfield(g_LuaState, -2, "name");
    lua_pushstring(g_LuaState, LOADER_VERSION);
    lua_setfield(g_LuaState, -2, "version");
    lua_pushstring(g_LuaState, LOADER_AUTHOR);
    lua_setfield(g_LuaState, -2, "author");
    lua_pushstring(g_LuaState, LOADER_EMAIL);
    lua_setfield(g_LuaState, -2, "email");
    lua_setglobal(g_LuaState, "LOADER");

    if (!std::filesystem::exists(scriptsDir)) {
        std::filesystem::create_directories(scriptsDir);
    }

    int loaded = 0;
    int failed = 0;

    for (const auto& entry : std::filesystem::directory_iterator(scriptsDir)) {
        if (!entry.is_regular_file() || entry.path().extension() != ".lua") continue;

        std::string filePath = entry.path().string();
        CrashHandler::WriteBreadcrumb(("load:" + entry.path().filename().string()).c_str());

        if (luaL_loadfile(g_LuaState, filePath.c_str()) != LUA_OK) {
            const char* err = lua_tostring(g_LuaState, -1);
            Logger::Log(LogLevel::Error, "load failed: " + filePath + " - " + (err ? err : "?"));
            lua_pop(g_LuaState, 1);
            failed++;
            continue;
        }
        if (lua_pcall(g_LuaState, 0, LUA_MULTRET, 0) != LUA_OK) {
            const char* err = lua_tostring(g_LuaState, -1);
            Logger::Log(LogLevel::Error, "exec failed: " + filePath + " - " + (err ? err : "?"));
            lua_pop(g_LuaState, 1);
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
    if (g_LuaState) {
        lua_close(g_LuaState);
        g_LuaState = nullptr;
        Logger::Log(LogLevel::Info, "Lua state closed");
    }
}
