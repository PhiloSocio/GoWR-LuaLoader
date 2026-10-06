#include "CrashHandler.h"
#include <dbghelp.h>
#include <fstream>

static volatile LONG g_CrashHandled = 0;
static char g_Breadcrumbs[16][64] = {};
static volatile LONG g_BreadcrumbIndex = 0;
static SRWLOCK g_BreadcrumbLock = SRWLOCK_INIT;

void CrashHandler::WriteBreadcrumb(const char* tag) {
    if (!tag) return;
    LONG idx = InterlockedIncrement(&g_BreadcrumbIndex) - 1;
    idx %= 16;
    AcquireSRWLockExclusive(&g_BreadcrumbLock);
    strncpy_s(g_Breadcrumbs[idx], tag, _TRUNCATE);
    ReleaseSRWLockExclusive(&g_BreadcrumbLock);
}

static bool IsFatalException(DWORD code) {
    switch (code) {
        case 0xC0000005:  // ACCESS_VIOLATION
        case 0xC000001D:  // ILLEGAL_INSTRUCTION
        case 0xC0000094:  // INTEGER_DIVIDE_BY_ZERO
        case 0xC00000FD:  // STACK_OVERFLOW
        case 0xC0000374:  // HEAP_CORRUPTION
        case 0xC0000409:  // STACK_BUFFER_OVERRUN
        case 0xC0000602:  // FAIL_FAST
        case 0xC000070A:  // ASSERTION_FAILURE
            return true;
        default:
            return false;
    }
}

static LONG WINAPI VectoredExceptionHandler(PEXCEPTION_POINTERS ExceptionInfo) {
    DWORD code = ExceptionInfo->ExceptionRecord->ExceptionCode;
    if (!IsFatalException(code)) {
        return EXCEPTION_CONTINUE_SEARCH;
    }
    if (InterlockedCompareExchange(&g_CrashHandled, 1, 0) != 0) {
        return EXCEPTION_CONTINUE_SEARCH;
    }

    auto context = ExceptionInfo->ContextRecord;
    auto record  = ExceptionInfo->ExceptionRecord;

    std::ofstream log("gowr_crash.log", std::ios::out);
    if (!log.is_open()) return EXCEPTION_CONTINUE_SEARCH;

    log << "================ GOW RAGNAROK CRASH LOG ================\n";
    log << "Exception Code: 0x" << std::hex << std::uppercase << record->ExceptionCode << "\n";
    log << "Exception Address: 0x" << record->ExceptionAddress << "\n";
    log << "Thread ID: " << std::dec << GetCurrentThreadId() << "\n\n";

    log << "Breadcrumbs (last 16):\n";
    for (int i = 0; i < 16; i++) {
        if (g_Breadcrumbs[i][0] == '\0') continue;
        log << "  [" << i << "] " << g_Breadcrumbs[i] << "\n";
    }
    log << "\n";

    log << "Registers:\n";
    log << "RAX: 0x" << context->Rax << "  RBX: 0x" << context->Rbx << "\n";
    log << "RCX: 0x" << context->Rcx << "  RDX: 0x" << context->Rdx << "\n";
    log << "RSI: 0x" << context->Rsi << "  RDI: 0x" << context->Rdi << "\n";
    log << "RBP: 0x" << context->Rbp << "  RSP: 0x" << context->Rsp << "\n";
    log << "RIP: 0x" << context->Rip << "\n";
    log << "R8:  0x" << context->R8  << "  R9:  0x" << context->R9  << "\n";
    log << "R10: 0x" << context->R10 << "  R11: 0x" << context->R11 << "\n";
    log << "R12: 0x" << context->R12 << "  R13: 0x" << context->R13 << "\n";
    log << "R14: 0x" << context->R14 << "  R15: 0x" << context->R15 << "\n\n";

    HANDLE process = GetCurrentProcess();
    SymSetOptions(SYMOPT_UNDNAME | SYMOPT_DEFERRED_LOADS | SYMOPT_LOAD_LINES);
    SymInitialize(process, NULL, TRUE);

    void* stack[64];
    USHORT frames = CaptureStackBackTrace(0, 64, stack, NULL);

    log << "Callstack:\n";
    for (USHORT i = 0; i < frames; i++) {
        DWORD64 address = (DWORD64)(stack[i]);
        char symbolBuffer[sizeof(SYMBOL_INFO) + MAX_SYM_NAME * sizeof(TCHAR)];
        PSYMBOL_INFO symbol = (PSYMBOL_INFO)symbolBuffer;
        symbol->SizeOfStruct = sizeof(SYMBOL_INFO);
        symbol->MaxNameLen = MAX_SYM_NAME;

        DWORD64 displacement = 0;
        DWORD lineDisplacement = 0;
        IMAGEHLP_LINE64 line = {};
        line.SizeOfStruct = sizeof(IMAGEHLP_LINE64);

        if (SymFromAddr(process, address, &displacement, symbol)) {
            log << "[" << i << "] " << symbol->Name << " + 0x" << std::hex << displacement;
            if (SymGetLineFromAddr64(process, address, &lineDisplacement, &line)) {
                log << "  (" << line.FileName << ":" << std::dec << line.LineNumber << ")";
            }
            log << "  [0x" << std::hex << address << "]\n";
        } else {
            log << "[" << i << "] 0x" << std::hex << address << "\n";
        }
    }

    SymCleanup(process);
    log.flush();
    log.close();

    return EXCEPTION_CONTINUE_SEARCH;
}

void CrashHandler::Initialize() {
    AddVectoredExceptionHandler(0, VectoredExceptionHandler);
}
