#pragma once

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

// Include the shared logger interface
// The path is relative because CMakeLists.txt adds src/shared to include directories
// but we need to be explicit here to avoid confusion with the local logger.h
#include <cstdint>

namespace suyan {
namespace log {

enum class Level {
    Debug = 0,
    Info = 1,
    Warning = 2,
    Error = 3
};

void initialize(const wchar_t* moduleName);
void shutdown();
void setLevel(Level level);

void debug(const char* fmt, ...);
void info(const char* fmt, ...);
void warning(const char* fmt, ...);
void error(const char* fmt, ...);

namespace detail {

HANDLE getLogFile();
void writeLog(Level level, const char* fmt, va_list args);

}  // namespace detail
}  // namespace log
}  // namespace suyan
