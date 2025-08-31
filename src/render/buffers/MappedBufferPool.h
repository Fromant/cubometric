#pragma once

#include <mutex>
#include <shared_mutex>
#include <unordered_map>

#include "Allocator.hpp"

namespace GPU {
    class MappedChunkBuffer {
    public:
        explicit MappedChunkBuffer(int capacity = 0) : allocator(capacity) {
            m_allocs.reserve(capacity);
        }

        Allocator::MemoryBlock getAllocation(size_t chunkId) {
            std::scoped_lock lock(mutex_);
            auto it = m_allocs.find(chunkId);
            if (it != m_allocs.end()) {
                return allocator[it->second];
            }
            return Allocator::MemoryBlock{0, 0, true};
        }

        Allocator::MemoryBlock allocate(size_t id, size_t capacity = 0) {
            std::scoped_lock lock(mutex_);
            auto [it, inserted] =
                m_allocs.try_emplace(id, allocator.alloc(capacity));
            return allocator[it->second];
        }

        void deallocate(size_t id) {
            std::scoped_lock lock(mutex_);
            auto it = m_allocs.find(id);
            if (it == m_allocs.end()) return;
            allocator.dealloc(it->second);
            m_allocs.erase(it);
        }

        bool containsAllocation(size_t id) const {
            std::shared_lock guard(mutex_); // Read-only, allow concurrent reads
            return m_allocs.contains(id);
        }

        void resizeAllocation(size_t id, size_t new_size) {
            std::scoped_lock lock(mutex_);
            auto it = m_allocs.find(id);
            if (it != m_allocs.end())
                it->second = allocator.realloc(it->second, new_size);
        }

        void flush() {
            std::scoped_lock lock(mutex_);
            allocator.flush();
        }

        void bind() {
            std::scoped_lock lock(mutex_);
            allocator.bind();
        }

        u64 get_capacity() const {
            std::shared_lock guard(mutex_);
            return allocator.get_capacity();
        }

        u64 get_used_memory() const {
            std::shared_lock guard(mutex_);
            return allocator.get_used_memory();
        }

        GLuint get_buffer() const {
            std::shared_lock guard(mutex_);
            return allocator.get_buffer();
        }

        void write(size_t id, void* data, size_t size, size_t offset) {
            std::scoped_lock lock(mutex_);
            auto it = m_allocs.find(id);
            if (it != m_allocs.end()) {
                const auto& view = allocator[it->second];
                if (view.is_free) return;
                if (size + offset > view.size)
                    throw std::runtime_error(
                        "write exceeds the limits of allocation");
                allocator.write(data, size, view.offset + offset);
            }
        }

        struct GPUBufferView {
            size_t offset = 0;
            size_t size = 0;
        };

        typedef std::array<std::array<GPUBufferView, 6>, Chunk::SUB_COUNT>
        ChunkBufferView;

        // Note: Returns a reference, so caller must not keep it across unlocks!
        ChunkBufferView& getChunkViewData(u64 id) {
            std::scoped_lock lock(mutex_);
            return chunkViewData[id]; // Creates entry if not exists
        }

        // If you want to allow reading ChunkBufferView without blocking writes,
        // you'd need separate synchronization (e.g., per-entry mutex or copy semantics).
        // As-is, this is safe only if used within the mutex scope.

    private:
        mutable std::shared_mutex mutex_; // Allows multiple readers, one writer
        std::unordered_map<u64, ChunkBufferView> chunkViewData;
        std::unordered_map<size_t, size_t> m_allocs;
        Allocator allocator;

        // Using std::shared_mutex allows const methods like get_capacity() to run concurrently.
        // If your compiler/stdlib doesn't support it (C++17+), use std::mutex instead.
    };
} // namespace GPU
