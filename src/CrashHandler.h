#pragma once

#include <windows.h>

namespace CrashHandler {
    void Initialize();
    void WriteBreadcrumb(const char* tag);
}
