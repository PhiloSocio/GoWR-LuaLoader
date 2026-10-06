#include "LuaManager.h"
#include "Logger.h"
#include "CrashHandler.h"
extern "C" {
    #include <lua.h>
    #include <lualib.h>
    #include <lauxlib.h>
}
#include <set>

static lua_State* g_LuaState = nullptr;
static std::mutex g_LuaMutex;

static int LuaPrint(lua_State* L) {
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

static int LuaLogInfo(lua_State* L)  { Logger::Log(LogLevel::Info,    std::string("[lua] ") + luaL_checkstring(L, 1)); return 0; }
static int LuaLogWarn(lua_State* L)  { Logger::Log(LogLevel::Warning, std::string("[lua] ") + luaL_checkstring(L, 1)); return 0; }
static int LuaLogError(lua_State* L) { Logger::Log(LogLevel::Error,   std::string("[lua] ") + luaL_checkstring(L, 1)); return 0; }
static int LuaLogDebug(lua_State* L) { Logger::Log(LogLevel::Debug,   std::string("[lua] ") + luaL_checkstring(L, 1)); return 0; }

static void AppendPackagePath(lua_State* L, const std::filesystem::path& root) {
    std::string rootStr = root.string();
    for (auto& c : rootStr) if (c == '\\') c = '/';

    std::string addition = rootStr + "/?.lua;" + rootStr + "/?/init.lua";

    lua_getglobal(L, "package");
    if (!lua_istable(L, -1)) { lua_pop(L, 1); return; }

    lua_getfield(L, -1, "path");
    std::string existing = lua_tostring(L, -1) ? lua_tostring(L, -1) : "";
    lua_pop(L, 1);

    std::string combined = existing.empty() ? addition : (addition + ";" + existing);

    lua_pushstring(L, combined.c_str());
    lua_setfield(L, -2, "path");
    lua_pop(L, 1);
}

static void CollectScripts(const std::filesystem::path& root, bool recursive,
                           std::set<std::filesystem::path>& seen,
                           std::vector<std::filesystem::path>& out) {
    if (!std::filesystem::exists(root) || !std::filesystem::is_directory(root)) return;

    auto consider = [&](const std::filesystem::directory_entry& e) {
        if (!e.is_regular_file()) return;
        if (e.path().extension() != ".lua") return;

        std::error_code ec;
        auto canon = std::filesystem::weakly_canonical(e.path(), ec);
        if (ec) canon = e.path();

        if (seen.insert(canon).second) out.push_back(e.path());
    };

    if (recursive) {
        for (const auto& e : std::filesystem::recursive_directory_iterator(
                 root, std::filesystem::directory_options::skip_permission_denied)) {
            consider(e);
        }
    } else {
        for (const auto& e : std::filesystem::directory_iterator(root)) {
            consider(e);
        }
    }
}

void LuaManager::Initialize(const std::vector<std::filesystem::path>& scriptRoots, bool recursive) {
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

    for (auto it = scriptRoots.rbegin(); it != scriptRoots.rend(); it++) {
        if (!std::filesystem::exists(*it)) {
            std::error_code ec;
            std::filesystem::create_directories(*it, ec);
        }
        AppendPackagePath(g_LuaState, *it);
    }

    std::set<std::filesystem::path> seen;
    std::vector<std::filesystem::path> files;
    for (const auto& root : scriptRoots) {
        CollectScripts(root, recursive, seen, files);
    }
    std::sort(files.begin(), files.end());

    int loaded = 0;
    int failed = 0;

    for (const auto& file : files) {
        auto parentName = file.parent_path().filename().string();
        std::string tag = parentName.empty()
            ? ("load: " + file.filename().string())
            : ("load: " + parentName + "/" + file.filename().string());
        CrashHandler::WriteBreadcrumb(tag.c_str());

        std::string filePath = file.string();
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

    Logger::Log(LogLevel::Info,
        "Lua scripts: " + std::to_string(loaded) + " loaded, " + std::to_string(failed) + " failed");
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
