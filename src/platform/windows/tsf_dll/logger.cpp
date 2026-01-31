#include "logger.h"
#include <shlobj.h>
#include <cstdio>
#include <cstdarg>
#include <ctime>

namespace suyan {
namespace log {

namespace {

HANDLE g_logFile = INVALID_HANDLE_VALUE;
Level g_level = Level::Info;
CRITICAL_SECTION g_cs;
bool g_csInitialized = false;
wchar_t g_moduleName[64] = L"tsf_dll";

const char* levelToString(Level level) {
    switch (level) {
        case Level::Debug:   return "DEBUG";
        case Level::Info:    return "INFO";
        case Level::Warning: return "WARN";
        case Level::Error:   return "ERROR";
        default:             return "UNKNOWN";
    }
}

bool ensureLogDirectory(wchar_t* path, size_t pathSize) {
    if (FAILED(SHGetFolderPathW(nullptr, CSIDL_APPDATA, nullptr, 0, path))) {
        return false;
    }
    wcscat_s(path, pathSize, L"\\SuYan\\logs");
    
    wchar_t* p = path + 3;
    while (*p) {
        if (*p == L'\\') {
            *p = L'\0';
            CreateDirectoryW(path, nullptr);
            *p = L'\\';
        }
        p++;
    }
    CreateDirectoryW(path, nullptr);
    return true;
}

}  // namespace

void initialize(const wchar_t* moduleName) {
    if (g_logFile != INVALID_HANDLE_VALUE) {
        return;
    }

    if (!g_csInitialized) {
        InitializeCriticalSection(&g_cs);
        g_csInitialized = true;
    }

    if (moduleName) {
        wcscpy_s(g_moduleName, moduleName);
    }

    wchar_t logPath[MAX_PATH];
    if (!ensureLogDirectory(logPath, MAX_PATH)) {
        return;
    }

    wchar_t logFile[MAX_PATH];
    swprintf_s(logFile, L"%s\\%s.log", logPath, g_moduleName);

    g_logFile = CreateFileW(
        logFile,
        FILE_APPEND_DATA,
        FILE_SHARE_READ | FILE_SHARE_WRITE,
        nullptr,
        OPEN_ALWAYS,
        FILE_ATTRIBUTE_NORMAL,
        nullptr
    );
}

void shutdown() {
    if (g_logFile != INVALID_HANDLE_VALUE) {
        CloseHandle(g_logFile);
        g_logFile = INVALID_HANDLE_VALUE;
    }
    if (g_csInitialized) {
        DeleteCriticalSection(&g_cs);
        g_csInitialized = false;
    }
}

void setLevel(Level level) {
    g_level = level;
}

namespace detail {

HANDLE getLogFile() {
    return g_logFile;
}

void writeLog(Level level, const char* fmt, va_list args) {
    if (level < g_level || g_logFile == INVALID_HANDLE_VALUE) {
        return;
    }

    EnterCriticalSection(&g_cs);

    SYSTEMTIME st;
    GetLocalTime(&st);

    char buffer[4096];
    int offset = snprintf(buffer, sizeof(buffer),
        "[%04d-%02d-%02d %02d:%02d:%02d.%03d] [%s] [PID:%lu TID:%lu] ",
        st.wYear, st.wMonth, st.wDay,
        st.wHour, st.wMinute, st.wSecond, st.wMilliseconds,
        levelToString(level),
        GetCurrentProcessId(),
        GetCurrentThreadId()
    );

    offset += vsnprintf(buffer + offset, sizeof(buffer) - offset, fmt, args);
    
    if (offset < (int)sizeof(buffer) - 1) {
        buffer[offset++] = '\n';
    }

    DWORD written;
    WriteFile(g_logFile, buffer, offset, &written, nullptr);
    FlushFileBuffers(g_logFile);

    LeaveCriticalSection(&g_cs);
}

}  // namespace detail

void debug(const char* fmt, ...) {
    va_list args;
    va_start(args, fmt);
    detail::writeLog(Level::Debug, fmt, args);
    va_end(args);
}

void info(const char* fmt, ...) {
    va_list args;
    va_start(args, fmt);
    detail::writeLog(Level::Info, fmt, args);
    va_end(args);
}

void warning(const char* fmt, ...) {
    va_list args;
    va_start(args, fmt);
    detail::writeLog(Level::Warning, fmt, args);
    va_end(args);
}

void error(const char* fmt, ...) {
    va_list args;
    va_start(args, fmt);
    detail::writeLog(Level::Error, fmt, args);
    va_end(args);
}

}  // namespace log
}  // namespace suyan
