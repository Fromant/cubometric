#pragma once

#include <unordered_map>

#include "Allocator.hpp"

namespace GPU {

class MappedChunkBuffer {
   public:
    explicit MappedChunkBuffer(int capacity = 0) : allocator(capacity) {
        m_allocs.reserve(capacity);
    }

    Allocator::MemoryBlock getAllocation(size_t chunkId) {
        auto it = m_allocs.find(chunkId);
        if (it != m_allocs.end()) {
            return allocator[it->second];
        }
        return Allocator::MemoryBlock{0, 0, true};
    }

    Allocator::MemoryBlock allocate(size_t id, size_t capacity = 0) {
        auto [it, inserted] =
            m_allocs.try_emplace(id, allocator.alloc(capacity));
        return allocator[it->second];
    }

    void deallocate(size_t id) {
        auto it = m_allocs.find(id);
        if (it == m_allocs.end()) return;
        allocator.dealloc(it->second);
        m_allocs.erase(it);
    }

    bool containsAllocation(size_t id) {
        return m_allocs.find(id) != m_allocs.end();
    }

    void resizeAllocation(size_t id, size_t new_size) {
        auto it = m_allocs.find(id);
        if (it != m_allocs.end())
            it->second = allocator.realloc(it->second, new_size);
    }

    void sync() { allocator.sync(); }
    void bind() { allocator.bind(); }
    u64 get_capacity() const { return allocator.get_capacity(); }
    u64 get_used_memory() const { return allocator.get_used_memory(); }
    GLuint get_buffer() const { return allocator.get_buffer(); }

    void write(size_t id, void* data, size_t size, size_t offset) {
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

    // TODO move from global please
    // global contents
    struct GPUBufferView {
        size_t offset = 0;
        size_t size = 0;
    };
    typedef std::array<std::array<GPUBufferView, 6>, Chunk::SUB_COUNT>
        ChunkBufferView;

    std::unordered_map<u64, ChunkBufferView> chunkViewData;

   private:
    std::unordered_map<size_t, size_t> m_allocs;
    Allocator allocator;
};
}  // namespace GPU
