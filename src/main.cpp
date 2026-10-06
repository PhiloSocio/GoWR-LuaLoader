#include "CrashHandler.h"
#include "LuaManager.h"
#include "Logger.h"
#include "Config.h"

HMODULE g_RealVersionDll = NULL;
HANDLE  g_ShutdownEvent  = NULL;
HANDLE  g_MainThread     = NULL;

extern "C" {
    FARPROC oGetFileVersionInfoA = NULL;
    FARPROC oGetFileVersionInfoByHandle = NULL;
    FARPROC oGetFileVersionInfoExA = NULL;
    FARPROC oGetFileVersionInfoExW = NULL;
    FARPROC oGetFileVersionInfoSizeA = NULL;
    FARPROC oGetFileVersionInfoSizeExA = NULL;
    FARPROC oGetFileVersionInfoSizeExW = NULL;
    FARPROC oGetFileVersionInfoSizeW = NULL;
    FARPROC oGetFileVersionInfoW = NULL;
    FARPROC oVerFindFileA = NULL;
    FARPROC oVerFindFileW = NULL;
    FARPROC oVerQueryValueA = NULL;
    FARPROC oVerQueryValueW = NULL;
}

static std::filesystem::path GetGameRoot() {
    char buffer[MAX_PATH];
    GetModuleFileNameA(NULL, buffer, MAX_PATH);
    return std::filesystem::path(buffer).parent_path();
}

static FARPROC ResolveExport(const char* name) {
    FARPROC p = GetProcAddress(g_RealVersionDll, name);
    if (!p) {
        char buf[256];
        sprintf_s(buf, "[GoWR-LuaLoader] GetProcAddress failed: %s\n", name);
        OutputDebugStringA(buf);
    }
    return p;
}

static void SetupProxy() {
    char systemPath[MAX_PATH];
    GetSystemDirectoryA(systemPath, MAX_PATH);
    std::string realDllPath = std::string(systemPath) + "\\version.dll";
    g_RealVersionDll = LoadLibraryA(realDllPath.c_str());

    if (!g_RealVersionDll) {
        OutputDebugStringA("[GoWR-LuaLoader] Failed to load system version.dll\n");
        return;
    }

    oGetFileVersionInfoA        = ResolveExport("GetFileVersionInfoA");
    oGetFileVersionInfoByHandle = ResolveExport("GetFileVersionInfoByHandle");
    oGetFileVersionInfoExA      = ResolveExport("GetFileVersionInfoExA");
    oGetFileVersionInfoExW      = ResolveExport("GetFileVersionInfoExW");
    oGetFileVersionInfoSizeA    = ResolveExport("GetFileVersionInfoSizeA");
    oGetFileVersionInfoSizeExA  = ResolveExport("GetFileVersionInfoSizeExA");
    oGetFileVersionInfoSizeExW  = ResolveExport("GetFileVersionInfoSizeExW");
    oGetFileVersionInfoSizeW    = ResolveExport("GetFileVersionInfoSizeW");
    oGetFileVersionInfoW        = ResolveExport("GetFileVersionInfoW");
    oVerFindFileA               = ResolveExport("VerFindFileA");
    oVerFindFileW               = ResolveExport("VerFindFileW");
    oVerQueryValueA             = ResolveExport("VerQueryValueA");
    oVerQueryValueW             = ResolveExport("VerQueryValueW");
}

static DWORD WINAPI MainThread(LPVOID) {
    auto gameRoot   = GetGameRoot();
    auto modDir     = gameRoot       / "mods";
    auto configPath = modDir        / "loader_config.toml";
    auto scripts    = modDir       / "scripts";
    auto logs       = modDir      / "logs";

    LoaderConfig config;
    std::string configError;
    bool configOk = LoadConfig(configPath, config, configError);

    Logger::Initialize(logs, config.ConsoleLogLevel, config.FileLogLevel);

    Logger::Log(LogLevel::Info, "GoWR-LuaLoader by AnArchos");
    Logger::Log(LogLevel::Info, "Credit: Nukem9 (gameplay-tweaks), Eiton (GoWR-Script-Loader)");
    Logger::Log(LogLevel::Info, "and, special thanks to Darthrolton, I did not have a plan to do this until his request.");

    if (!configOk) {
        Logger::Log(LogLevel::Warning, "Using defaults. Reason: " + configError);
    } else {
        Logger::Log(LogLevel::Info, "Config loaded: " + configPath.string());
    }

    if (!g_RealVersionDll) {
        Logger::Log(LogLevel::Error, "System version.dll was not loaded; proxy exports are null and the game may crash on first version API call");
    }

    CrashHandler::Initialize();

    Logger::Log(LogLevel::Info, "GoWR LuaLoader starting");
    Logger::Log(LogLevel::Info, "GameRoot: " + gameRoot.string());

    if (config.LoadScripts) {
        LuaManager::Initialize(scripts);
    }

    Logger::Log(LogLevel::Info, "Waiting for shutdown signal");

    WaitForSingleObject(g_ShutdownEvent, INFINITE);

    Logger::Log(LogLevel::Info, "Shutdown signal received");
    LuaManager::Shutdown();
    Logger::Log(LogLevel::Info, "Goodbye");
    Logger::Shutdown();
    return 0;
}

BOOL APIENTRY DllMain(HMODULE hModule, DWORD reason, LPVOID) {
    switch (reason) {
    case DLL_PROCESS_ATTACH:
        DisableThreadLibraryCalls(hModule);
        SetupProxy();
        g_ShutdownEvent = CreateEventW(NULL, TRUE, FALSE, NULL);
        g_MainThread    = CreateThread(NULL, 0, MainThread, NULL, 0, NULL);
        break;
    case DLL_PROCESS_DETACH:
        if (g_ShutdownEvent) {
            SetEvent(g_ShutdownEvent);
            if (g_MainThread) {
                WaitForSingleObject(g_MainThread, 3000);
            }
        }
        if (g_MainThread)     CloseHandle(g_MainThread);
        if (g_ShutdownEvent)  CloseHandle(g_ShutdownEvent);
        if (g_RealVersionDll) FreeLibrary(g_RealVersionDll);
        break;
    }
    return TRUE;
}
