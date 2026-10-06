#include "CrashHandler.h"
#include "LuaManager.h"
#include "Logger.h"
#include "Config.h"

HANDLE g_ShutdownEvent = NULL;
HANDLE g_MainThread    = NULL;

static std::filesystem::path GetGameRoot() {
    char buffer[MAX_PATH];
    GetModuleFileNameA(NULL, buffer, MAX_PATH);
    return std::filesystem::path(buffer).parent_path();
}

static DWORD WINAPI MainThread(LPVOID) {
    auto gameRoot   = GetGameRoot();
    auto modDir     = gameRoot  / "mods";
    auto configPath = modDir   / "loader_config.toml";
    auto logs       = modDir  / "logs";

    LoaderConfig config;
    std::string configError;
    bool configOk = LoadConfig(configPath, config, configError);

    Logger::Initialize(logs, config.ConsoleLogLevel, config.FileLogLevel);

    Logger::Log(LogLevel::Info, std::string(LOADER_NAME) + " v" + LOADER_VERSION + " by " + LOADER_AUTHOR);
    Logger::Log(LogLevel::Info, "Credits: Nukem9 (gameplay-tweaks), Eiton (GoWR-Script-Loader)");
    Logger::Log(LogLevel::Info, "Special thanks: Darthrolton, whose request started this project");

    if (!configOk) {
        Logger::Log(LogLevel::Warning, "Using defaults. Reason: " + configError);
    } else {
        Logger::Log(LogLevel::Info, "Config loaded: " + configPath.string());
    }

    CrashHandler::Initialize();

    Logger::Log(LogLevel::Info, std::string(LOADER_NAME) + " starting");
    Logger::Log(LogLevel::Info, "GameRoot: " + gameRoot.string());

    if (config.LoadScripts) {
        std::vector<std::filesystem::path> scriptRoots;
        for (const auto& rel : config.ScriptRoots) {
            auto fullPath = (gameRoot / rel).lexically_normal();
            scriptRoots.push_back(fullPath);
            Logger::Log(LogLevel::Info, "Script root: " + fullPath.string());
        }
        LuaManager::Initialize(scriptRoots, config.ScanScriptsRecursively);
    }

    Logger::Log(LogLevel::Info, "Waiting for shutdown signal");

    WaitForSingleObject(g_ShutdownEvent, INFINITE);

    Logger::Log(LogLevel::Info, "Shutdown signal received");
    LuaManager::Shutdown();
    Logger::Log(LogLevel::Info, "Goodbye");
    Logger::Shutdown();
    return 0;
}

extern "C" void LoaderStartup() {
    g_ShutdownEvent = CreateEventW(NULL, TRUE, FALSE, NULL);
    g_MainThread    = CreateThread(NULL, 0, MainThread, NULL, 0, NULL);
}

extern "C" void LoaderShutdown() {
    if (g_ShutdownEvent) {
        SetEvent(g_ShutdownEvent);
        if (g_MainThread) WaitForSingleObject(g_MainThread, 3000);
    }
    if (g_MainThread)    CloseHandle(g_MainThread);
    if (g_ShutdownEvent) CloseHandle(g_ShutdownEvent);
}
