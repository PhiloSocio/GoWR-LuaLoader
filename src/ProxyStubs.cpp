#include <windows.h>
#include <string>

extern "C" void LoaderStartup();
extern "C" void LoaderShutdown();

static HMODULE g_OriginalLib = nullptr;

using FnGetFileVersionInfoA        = BOOL  (WINAPI*)(LPCSTR, DWORD, DWORD, LPVOID);
using FnGetFileVersionInfoByHandle = BOOL  (WINAPI*)(DWORD, HANDLE, LPVOID, DWORD);
using FnGetFileVersionInfoExA      = BOOL  (WINAPI*)(DWORD, LPCSTR, DWORD, DWORD, LPVOID);
using FnGetFileVersionInfoExW      = BOOL  (WINAPI*)(DWORD, LPCWSTR, DWORD, DWORD, LPVOID);
using FnGetFileVersionInfoSizeA    = DWORD (WINAPI*)(LPCSTR, LPDWORD);
using FnGetFileVersionInfoSizeExA  = DWORD (WINAPI*)(DWORD, LPCSTR, LPDWORD);
using FnGetFileVersionInfoSizeExW  = DWORD (WINAPI*)(DWORD, LPCWSTR, LPDWORD);
using FnGetFileVersionInfoSizeW    = DWORD (WINAPI*)(LPCWSTR, LPDWORD);
using FnGetFileVersionInfoW        = BOOL  (WINAPI*)(LPCWSTR, DWORD, DWORD, LPVOID);
using FnVerFindFileA               = DWORD (WINAPI*)(DWORD, LPCSTR, LPCSTR, LPCSTR, LPSTR, PUINT, LPSTR, PUINT);
using FnVerFindFileW               = DWORD (WINAPI*)(DWORD, LPCWSTR, LPCWSTR, LPCWSTR, LPWSTR, PUINT, LPWSTR, PUINT);
using FnVerQueryValueA             = BOOL  (WINAPI*)(LPCVOID, LPCSTR, LPVOID*, PUINT);
using FnVerQueryValueW             = BOOL  (WINAPI*)(LPCVOID, LPCWSTR, LPVOID*, PUINT);

static FnGetFileVersionInfoA        pGetFileVersionInfoA        = nullptr;
static FnGetFileVersionInfoByHandle pGetFileVersionInfoByHandle = nullptr;
static FnGetFileVersionInfoExA      pGetFileVersionInfoExA      = nullptr;
static FnGetFileVersionInfoExW      pGetFileVersionInfoExW      = nullptr;
static FnGetFileVersionInfoSizeA    pGetFileVersionInfoSizeA    = nullptr;
static FnGetFileVersionInfoSizeExA  pGetFileVersionInfoSizeExA  = nullptr;
static FnGetFileVersionInfoSizeExW  pGetFileVersionInfoSizeExW  = nullptr;
static FnGetFileVersionInfoSizeW    pGetFileVersionInfoSizeW    = nullptr;
static FnGetFileVersionInfoW        pGetFileVersionInfoW        = nullptr;
static FnVerFindFileA               pVerFindFileA               = nullptr;
static FnVerFindFileW               pVerFindFileW               = nullptr;
static FnVerQueryValueA             pVerQueryValueA             = nullptr;
static FnVerQueryValueW             pVerQueryValueW             = nullptr;

extern "C" {

BOOL WINAPI ProxyGetFileVersionInfoA(LPCSTR a, DWORD b, DWORD c, LPVOID d) {
    return pGetFileVersionInfoA(a, b, c, d);
}

BOOL WINAPI ProxyGetFileVersionInfoByHandle(DWORD a, HANDLE b, LPVOID c, DWORD d) {
    return pGetFileVersionInfoByHandle(a, b, c, d);
}

BOOL WINAPI ProxyGetFileVersionInfoExA(DWORD a, LPCSTR b, DWORD c, DWORD d, LPVOID e) {
    return pGetFileVersionInfoExA(a, b, c, d, e);
}

BOOL WINAPI ProxyGetFileVersionInfoExW(DWORD a, LPCWSTR b, DWORD c, DWORD d, LPVOID e) {
    return pGetFileVersionInfoExW(a, b, c, d, e);
}

DWORD WINAPI ProxyGetFileVersionInfoSizeA(LPCSTR a, LPDWORD b) {
    return pGetFileVersionInfoSizeA(a, b);
}

DWORD WINAPI ProxyGetFileVersionInfoSizeExA(DWORD a, LPCSTR b, LPDWORD c) {
    return pGetFileVersionInfoSizeExA(a, b, c);
}

DWORD WINAPI ProxyGetFileVersionInfoSizeExW(DWORD a, LPCWSTR b, LPDWORD c) {
    return pGetFileVersionInfoSizeExW(a, b, c);
}

DWORD WINAPI ProxyGetFileVersionInfoSizeW(LPCWSTR a, LPDWORD b) {
    return pGetFileVersionInfoSizeW(a, b);
}

BOOL WINAPI ProxyGetFileVersionInfoW(LPCWSTR a, DWORD b, DWORD c, LPVOID d) {
    return pGetFileVersionInfoW(a, b, c, d);
}

DWORD WINAPI ProxyVerFindFileA(DWORD a, LPCSTR b, LPCSTR c, LPCSTR d, LPSTR e, PUINT f, LPSTR g, PUINT h) {
    return pVerFindFileA(a, b, c, d, e, f, g, h);
}

DWORD WINAPI ProxyVerFindFileW(DWORD a, LPCWSTR b, LPCWSTR c, LPCWSTR d, LPWSTR e, PUINT f, LPWSTR g, PUINT h) {
    return pVerFindFileW(a, b, c, d, e, f, g, h);
}

BOOL WINAPI ProxyVerQueryValueA(LPCVOID a, LPCSTR b, LPVOID* c, PUINT d) {
    return pVerQueryValueA(a, b, c, d);
}

BOOL WINAPI ProxyVerQueryValueW(LPCVOID a, LPCWSTR b, LPVOID* c, PUINT d) {
    return pVerQueryValueW(a, b, c, d);
}

} // extern "C"

static void ResolveAllExports(HMODULE lib) {
    pGetFileVersionInfoA        = (FnGetFileVersionInfoA)        GetProcAddress(lib, "GetFileVersionInfoA");
    pGetFileVersionInfoByHandle = (FnGetFileVersionInfoByHandle) GetProcAddress(lib, "GetFileVersionInfoByHandle");
    pGetFileVersionInfoExA      = (FnGetFileVersionInfoExA)      GetProcAddress(lib, "GetFileVersionInfoExA");
    pGetFileVersionInfoExW      = (FnGetFileVersionInfoExW)      GetProcAddress(lib, "GetFileVersionInfoExW");
    pGetFileVersionInfoSizeA    = (FnGetFileVersionInfoSizeA)    GetProcAddress(lib, "GetFileVersionInfoSizeA");
    pGetFileVersionInfoSizeExA  = (FnGetFileVersionInfoSizeExA)  GetProcAddress(lib, "GetFileVersionInfoSizeExA");
    pGetFileVersionInfoSizeExW  = (FnGetFileVersionInfoSizeExW)  GetProcAddress(lib, "GetFileVersionInfoSizeExW");
    pGetFileVersionInfoSizeW    = (FnGetFileVersionInfoSizeW)    GetProcAddress(lib, "GetFileVersionInfoSizeW");
    pGetFileVersionInfoW        = (FnGetFileVersionInfoW)        GetProcAddress(lib, "GetFileVersionInfoW");
    pVerFindFileA               = (FnVerFindFileA)               GetProcAddress(lib, "VerFindFileA");
    pVerFindFileW               = (FnVerFindFileW)               GetProcAddress(lib, "VerFindFileW");
    pVerQueryValueA             = (FnVerQueryValueA)             GetProcAddress(lib, "VerQueryValueA");
    pVerQueryValueW             = (FnVerQueryValueW)             GetProcAddress(lib, "VerQueryValueW");
}

static bool AllExportsResolved() {
    return pGetFileVersionInfoA        && pGetFileVersionInfoByHandle &&
           pGetFileVersionInfoExA      && pGetFileVersionInfoExW     &&
           pGetFileVersionInfoSizeA    && pGetFileVersionInfoSizeExA &&
           pGetFileVersionInfoSizeExW  && pGetFileVersionInfoSizeW   &&
           pGetFileVersionInfoW        && pVerFindFileA              &&
           pVerFindFileW               && pVerQueryValueA            &&
           pVerQueryValueW;
}

extern "C" BOOL WINAPI DllMain(HMODULE hModule, DWORD reason, LPVOID) {
    if (reason == DLL_PROCESS_ATTACH) {
        DisableThreadLibraryCalls(hModule);

        char systemPath[MAX_PATH];
        GetSystemDirectoryA(systemPath, MAX_PATH);
        std::string fullPath = std::string(systemPath) + "\\version.dll";

        g_OriginalLib = LoadLibraryA(fullPath.c_str());
        if (!g_OriginalLib) {
            MessageBoxA(nullptr, "GoWR-LuaLoader: failed to load system version.dll", "Proxy Error", MB_ICONERROR);
            ExitProcess(1);
        }

        ResolveAllExports(g_OriginalLib);
        if (!AllExportsResolved()) {
            MessageBoxA(nullptr, "GoWR-LuaLoader: one or more version.dll exports could not be resolved", "Proxy Error", MB_ICONERROR);
            ExitProcess(1);
        }

        LoaderStartup();
    }
    else if (reason == DLL_PROCESS_DETACH) {
        LoaderShutdown();
        if (g_OriginalLib) {
            FreeLibrary(g_OriginalLib);
            g_OriginalLib = nullptr;
        }
    }
    return TRUE;
}
