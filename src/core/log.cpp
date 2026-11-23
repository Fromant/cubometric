#include "core/log.h"

#include <chrono>
#include <cstdarg>
#include <cstdio>
#include <ctime>
#include <mutex>

namespace ih::core {

namespace {

std::mutex g_logMutex;

void vwrite (LogLevel level, const char* fmt, std::va_list args) {
    char message[2048];
    std::vsnprintf (message, sizeof (message), fmt, args);

    // Cheap wall-clock timestamp, ms resolution, no timezone round trips.
    const auto now = std::chrono::system_clock::now().time_since_epoch ();
    const auto ms = static_cast<int> (std::chrono::duration_cast<std::chrono::milliseconds> (now).count () % 1000);
    const auto t = static_cast<time_t> (std::chrono::duration_cast<std::chrono::seconds> (now).count ());
    std::tm tm {};
#ifdef _WIN32
    localtime_s (&tm, &t);
#else
    localtime_r (&t, &tm);
#endif

    const char* tag = level == LogLevel::Info ? "i" : (level == LogLevel::Warn ? "w" : "E");
    std::lock_guard lock (g_logMutex);
    std::fprintf (stderr, "%02d:%02d:%02d.%03d [%s] %s\n", tm.tm_hour, tm.tm_min, tm.tm_sec, ms, tag, message);
}

}  // namespace

void logf (LogLevel level, const char* fmt, ...) noexcept {
    std::va_list args;
    va_start (args, fmt);
    vwrite (level, fmt, args);
    va_end (args);
}

void log (LogLevel level, const std::string& message) { logf (level, "%s", message.c_str ()); }

void logInfo (const std::string& message) { log (LogLevel::Info, message); }
void logWarn (const std::string& message) { log (LogLevel::Warn, message); }
void logError (const std::string& message) { log (LogLevel::Error, message); }

}  // namespace ih::core
