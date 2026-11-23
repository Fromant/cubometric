#pragma once

#include <chrono>
#include <condition_variable>
#include <cstdint>
#include <mutex>

#include <glm/glm.hpp>

#include "core/frustum.h"

namespace ih::core {

// Per-frame state pushed from the event thread to the render thread.
// Latest-wins: intermediate updates are coalesced.
struct FrameParams {
    uint32_t width = 0;
    uint32_t height = 0;
    glm::vec3 cameraPos{0.0f};
    glm::mat4 viewProj{1.0f};
    Frustum frustum{};
    int renderDistance = 8;
    bool wireframe = false;
};

// Single-producer / single-consumer, latest-wins queue.
class FrameParamsQueue {
public:
    void post (const FrameParams& params) {
        std::lock_guard lock (mutex);
        latest = params;
        hasValue = true;
        cv.notify_all ();
    }

    // Blocks until a new value arrives or the timeout elapses.
    bool waitLatest (FrameParams& out, std::chrono::milliseconds timeout) {
        std::unique_lock lock (mutex);
        cv.wait_for (lock, timeout, [this] { return hasValue; });
        if (!hasValue) return false;
        out = latest;
        hasValue = false;
        return true;
    }

private:
    std::mutex mutex;
    std::condition_variable cv;
    FrameParams latest {};
    bool hasValue = false;
};

}  // namespace ih::core
