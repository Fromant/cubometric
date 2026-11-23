#pragma once

#include <cstdarg>
#include <string>

namespace ih::core {

enum class LogLevel { Info, Warn, Error };

// printf-style logging. Thread-safe.
void logf (LogLevel level, const char* fmt, ...) noexcept;
void log (LogLevel level, const std::string& message);

void logInfo (const std::string& message);
void logWarn (const std::string& message);
void logError (const std::string& message);

}  // namespace ih::core
