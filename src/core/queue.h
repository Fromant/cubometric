#pragma once

#include <atomic>
#include <cstddef>
#include <new>
#include <vector>

namespace ih::core {

// Bounded multi-producer / multi-consumer queue (Vyukov style).
// Fixed capacity, no allocations on the hot path, no locks.
// tryPush/tryPop return false when full/empty; callers decide how to wait.
template <typename T>
class BoundedQueue {
public:
    explicit BoundedQueue (size_t capacity) : m_mask (capacity - 1), m_cells (capacity) {
        // Capacity must be a power of two.
        for (size_t i = 0; i < capacity; ++i) m_cells[i].sequence.store (i, std::memory_order_relaxed);
    }

    BoundedQueue (const BoundedQueue&) = delete;
    BoundedQueue& operator= (const BoundedQueue&) = delete;

    bool tryPush (const T& value) {
        Cell* cell;
        size_t pos = m_enqueuePos.load (std::memory_order_relaxed);
        for (;;) {
            cell = &m_cells[pos & m_mask];
            const size_t seq = cell->sequence.load (std::memory_order_acquire);
            const intptr_t diff = static_cast<intptr_t> (seq) - static_cast<intptr_t> (pos);
            if (diff == 0) {
                if (m_enqueuePos.compare_exchange_weak (pos, pos + 1, std::memory_order_relaxed)) break;
            } else if (diff < 0) {
                return false;  // full
            } else {
                pos = m_enqueuePos.load (std::memory_order_relaxed);
            }
        }
        cell->data = value;
        cell->sequence.store (pos + 1, std::memory_order_release);
        return true;
    }

    bool tryPop (T& out) {
        Cell* cell;
        size_t pos = m_dequeuePos.load (std::memory_order_relaxed);
        for (;;) {
            cell = &m_cells[pos & m_mask];
            const size_t seq = cell->sequence.load (std::memory_order_acquire);
            const intptr_t diff = static_cast<intptr_t> (seq) - static_cast<intptr_t> (pos + 1);
            if (diff == 0) {
                if (m_dequeuePos.compare_exchange_weak (pos, pos + 1, std::memory_order_relaxed)) break;
            } else if (diff < 0) {
                return false;  // empty
            } else {
                pos = m_dequeuePos.load (std::memory_order_relaxed);
            }
        }
        out = std::move (cell->data);
        cell->sequence.store (pos + m_mask + 1, std::memory_order_release);
        return true;
    }

private:
    struct Cell {
        std::atomic<size_t> sequence{0};
        T data{};
    };

    const size_t m_mask;
    std::vector<Cell> m_cells;
    alignas (64) std::atomic<size_t> m_enqueuePos{0};
    alignas (64) std::atomic<size_t> m_dequeuePos{0};
};

}  // namespace ih::core
