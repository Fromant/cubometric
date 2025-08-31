#pragma once

#include <concurrentqueue.h>
#include <atomic>

#include "Allocator.hpp"

class ChunkMeshQueue {
public:
    // Notify that a chunk needs remeshing (called from render thread)
    void mark_for_meshing(const glm::ivec2& coords) {
        size_t idx = hash_to_index(Chunk::getId(coords.x, coords.y));

        // Fast path: already queued?
        if (dirty_flags_[idx].load(std::memory_order_relaxed)) {
            return; // Already in queue or pending
        }

        // Try to set flag
        if (dirty_flags_[idx].exchange(true, std::memory_order_acquire)) {
            return; // Another thread just set it
        }

        // Successfully claimed: enqueue
        queue_.enqueue(producer_token_, coords);
    }

    // Called by mesh thread(s)
    bool pop(glm::ivec2& coords) {
        return queue_.try_dequeue_from_producer(producer_token_, coords);
    }

    size_t pop_bulk(std::vector<glm::ivec2>& container) {
        return queue_.try_dequeue_bulk_from_producer(producer_token_, container.begin(), container.capacity());
    }

    void clear_flag(const glm::ivec2& pos) {
        size_t idx = hash_to_index(Chunk::getId(pos));
        dirty_flags_[idx].store(false, std::memory_order_release);
    }

    void clear_flag_bulk(const std::vector<glm::ivec2>& container) {
        for (const auto& pos : container) {
            clear_flag(pos);
        }
    }

    // Optional: batch pop
    template <typename Container>
    size_t pop_all(Container& out) {
        return queue_.try_dequeue_bulk(out);
    }

private:
    static constexpr size_t FLAG_ARRAY_SIZE = 1 << 16; // 65536
    alignas(64) std::atomic<bool> dirty_flags_[FLAG_ARRAY_SIZE]{};

    moodycamel::ConcurrentQueue<glm::ivec2> queue_;
    moodycamel::ProducerToken producer_token_{queue_};

    static size_t hash_to_index(u64 chunk_id) {
        chunk_id = (~chunk_id) + (chunk_id << 21); // Knuth mix
        chunk_id ^= (chunk_id >> 24);
        chunk_id = (chunk_id + (chunk_id << 3)) + (chunk_id << 8);
        chunk_id ^= (chunk_id >> 14);
        return chunk_id % FLAG_ARRAY_SIZE;
    }
};
