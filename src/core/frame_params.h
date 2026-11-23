#pragma once

#include <chrono>
#include <condition_variable>
#include <cstdint>
#include <mutex>

namespace ih::core {

// Per-frame state pushed from the event thread to the render thread.
// M0 carries the window size only; camera and input state join here later.
struct FrameParams {
    uint32_t width = 0;
    uint32_t height = 0;
};

// Single-producer / single-consumer, latest-wins queue.
// The consumer drains at most one value per poll; intermediate updates
// are coalesced, which is exactly what resize events need.
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
