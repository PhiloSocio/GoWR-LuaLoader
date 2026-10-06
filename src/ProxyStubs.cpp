#include <windows.h>

#define DLL_PROXY_EXPORT_LISTING_FILE "winmm_exports.inc"
#define DLL_PROXY_TLS_CALLBACK_AUTOINIT
#define DLL_PROXY_CHECK_MISSING_EXPORTS
#define DLL_PROXY_DECLARE_IMPLEMENTATION
#include <QuickDllProxy/DllProxy.h>

extern "C" void LoaderStartup();
extern "C" void LoaderShutdown();

BOOL APIENTRY DllMain(HMODULE hModule, DWORD reason, LPVOID) {
    switch (reason) {
        case DLL_PROCESS_ATTACH:
            DisableThreadLibraryCalls(hModule);
            LoaderStartup();
            break;

        case DLL_PROCESS_DETACH:
            LoaderShutdown();
            break;
    }
    return TRUE;
}
