#pragma once

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

}  // namespace log
}  // namespace suyan
