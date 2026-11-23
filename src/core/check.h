#pragma once

#include <cstdlib>
#include <string>
#include <vulkan/vulkan.h>

#include "core/log.h"

namespace ih {

[[noreturn]] inline void checkFail (const char* condition, const char* file, int line) {
    core::logError ("CHECK failed: " + std::string (condition) + " (" + file + ":" + std::to_string (line) + ")");
    std::abort ();
}

}  // namespace ih

// Aborts with a readable message if the condition is false.
#define IH_CHECK(cond)                                   \
    do {                                                 \
        if (!(cond)) ih::checkFail (#cond, __FILE__, __LINE__); \
    } while (0)

// Checks a Vulkan call that returns VkResult. (vku RAII constructors signal
// errors by throwing std::system_error instead, so they need no wrapper.)
#define VK_CHECK(expr)                                          \
    do {                                                        \
        const VkResult ih_r = (expr);                           \
        if (ih_r != VK_SUCCESS) {                               \
            ih::core::logError (std::string ("Vulkan error: ") + #expr + " -> " + std::to_string (static_cast<int> (ih_r))); \
            std::abort ();                                      \
        }                                                       \
    } while (0)
