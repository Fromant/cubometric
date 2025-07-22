#include <glad/glad.h>

#include <algorithm>
#include <list>
#include <stdexcept>
#include <vector>

#include "render/renderers/block/Vertex.h"

typedef unsigned long long u64;

namespace GPU {

class Allocator {
   public:
    struct MemoryBlock {
        u64 offset;
        u64 size;
        bool is_free;

        MemoryBlock(u64 offset, u64 size, bool is_free = true)
            : offset(offset), size(size), is_free(is_free) {}

        MemoryBlock() : offset(0), size(0), is_free(true) {}
    };

    // ban copying
    Allocator(const Allocator&) = delete;
    Allocator& operator=(const Allocator&) = delete;

   private:
    std::list<MemoryBlock> memory_blocks;
    std::vector<std::list<MemoryBlock>::iterator> allocations;
    GLuint buffer;
    u64 capacity;
    u64 used_memory;
    void* mapped_ptr;
    bool is_mapped;
    GLsync fence;

    static constexpr u64 INITIAL_CAPACITY = 1024 * 1024;  // 1mb
    static constexpr int ALIGNMENT = 256;
    static constexpr GLbitfield BUFFER_FLAGS =
        GL_MAP_WRITE_BIT | GL_MAP_PERSISTENT_BIT | GL_MAP_COHERENT_BIT |
        GL_DYNAMIC_STORAGE_BIT;
    static constexpr GLbitfield MAP_FLAGS =
        GL_MAP_WRITE_BIT | GL_MAP_PERSISTENT_BIT | GL_MAP_COHERENT_BIT |
        GL_MAP_FLUSH_EXPLICIT_BIT;

   public:
    Allocator(u64 initial_capacity = INITIAL_CAPACITY)
        : capacity(std::max(initial_capacity, INITIAL_CAPACITY)),
          used_memory(0),
          mapped_ptr(nullptr),
          is_mapped(false),
          fence(nullptr) {
        glCreateBuffers(1, &buffer);
        glNamedBufferStorage(buffer, capacity, nullptr, BUFFER_FLAGS);
        memory_blocks.emplace_back(0, capacity);

        // Persistent mapping
        mapped_ptr = glMapNamedBufferRange(buffer, 0, capacity, MAP_FLAGS);
        is_mapped = true;
    }

    ~Allocator() {
        if (fence) {
            glDeleteSync(fence);
        }
        if (is_mapped) {
            glUnmapNamedBuffer(buffer);
        }
        glDeleteBuffers(1, &buffer);
    }

    // Flushes any pending operations and ensures GPU visibility
    void sync() {
        if (fence) {
            glClientWaitSync(fence, 0, GL_TIMEOUT_IGNORED);
            glDeleteSync(fence);
            fence = nullptr;
        }

        // For persistent mapped buffers, we need to explicitly flush modified
        // ranges
        if (is_mapped) {
            // Since we don't track modified ranges, we flush the entire buffer
            // For better performance, you could track modified ranges
            glFlushMappedNamedBufferRange(buffer, 0, capacity);
        }

        // Insert a new fence for future synchronization
        fence = glFenceSync(GL_SYNC_GPU_COMMANDS_COMPLETE, 0);
    }

    size_t alloc(u64 size) {
        if (size == 0) return -1;

        size = align_size(size, ALIGNMENT);

        auto it = find_free_block(size);
        if (it == memory_blocks.end()) {
            resize(capacity * 2);
            return alloc(size);
        }

        return split_and_allocate(it, size);
    }

    size_t realloc(size_t allocation_id, u64 new_size) {
        if (allocation_id >= allocations.size() ||
            allocations[allocation_id]->is_free) {
            return -1;
        }

        if (new_size == 0) {
            dealloc(allocation_id);
            return -1;
        }

        new_size = align_size(new_size, ALIGNMENT);
        auto old_alloc = allocations[allocation_id];

        // If block is smaller, just update size
        if (new_size <= old_alloc->size) {
            old_alloc->size = new_size;
            return allocation_id;
        }

        // Try to expand block in place
        auto next_block = std::next(old_alloc);
        if (next_block != memory_blocks.end() && next_block->is_free) {
            u64 available_space = old_alloc->size + next_block->size;
            if (available_space >= new_size) {
                u64 remaining_space = available_space - new_size;

                // Merge blocks
                old_alloc->size = new_size;
                memory_blocks.erase(next_block);

                if (remaining_space > 0) {
                    memory_blocks.insert(
                        std::next(old_alloc),
                        MemoryBlock(old_alloc->offset + new_size,
                                    remaining_space, true));
                }

                used_memory += (new_size - old_alloc->size);
                return allocation_id;
            }
        }

        // If can't expand, allocate new block and copy data
        size_t new_allocation_id = alloc(new_size);
        if (new_allocation_id == -1) return -1;

        // Ensure any previous operations are complete before copying
        if (fence) {
            glClientWaitSync(fence, 0, GL_TIMEOUT_IGNORED);
        }

        memcpy(static_cast<char*>(mapped_ptr) +
                   allocations[new_allocation_id]->offset,
               static_cast<char*>(mapped_ptr) + old_alloc->offset,
               std::min(old_alloc->size, new_size));

        dealloc(allocation_id);
        return new_allocation_id;
    }

    void dealloc(size_t allocation_id) {
        if (allocation_id >= allocations.size() ||
            allocations[allocation_id]->is_free) {
            return;
        }

        auto& alloc = allocations[allocation_id];
        alloc->is_free = true;
        used_memory -= alloc->size;
        merge_adjacent_blocks();
    }

    void write(const void* data, size_t size, size_t offset) {
        if (!data || size == 0) return;

        if (offset + size > capacity) {
            throw std::out_of_range("Write operation exceeds buffer capacity");
        }

        if (is_mapped) {
            // Use mapped memory for writing
            memcpy(static_cast<char*>(mapped_ptr) + offset, data, size);
        } else {
            // Fallback to glBufferSubData
            glNamedBufferSubData(buffer, offset, size, data);
        }
    }

    void read(void* out_data, size_t size, size_t offset) {
        if (offset + size > capacity) {
            throw std::out_of_range("Read operation exceeds buffer capacity");
        }

        if (is_mapped) {
            // Ensure any pending writes are complete
            if (fence) {
                glClientWaitSync(fence, 0, GL_TIMEOUT_IGNORED);
            }

            // Use mapped memory for reading
            memcpy(out_data, static_cast<char*>(mapped_ptr) + offset, size);
        } else {
            // Fallback to glGetBufferSubData
            glGetNamedBufferSubData(buffer, offset, size, out_data);
        }
    }

    const MemoryBlock& operator[](size_t allocation_id) const {
        return *allocations[allocation_id];
    }

    void resize(u64 new_size) {
        if (new_size <= capacity) return;

        // Unmap current buffer if mapped
        if (is_mapped) {
            glUnmapNamedBuffer(buffer);
            is_mapped = false;
        }

        // Create new buffer
        GLuint new_buffer;
        glCreateBuffers(1, &new_buffer);
        glNamedBufferStorage(new_buffer, new_size, nullptr, BUFFER_FLAGS);

        // Map new buffer
        void* new_mapped_ptr =
            glMapNamedBufferRange(new_buffer, 0, new_size, MAP_FLAGS);

        // Copy data from old buffer
        if (used_memory > 0) {
            if (mapped_ptr) {
                // Fast path - copy via mapped pointers
                memcpy(new_mapped_ptr, mapped_ptr, capacity);
            } else {
                // Fallback to GPU copy
                glCopyNamedBufferSubData(buffer, new_buffer, 0, 0, capacity);
            }
        }

        // Clean up old buffer
        glDeleteBuffers(1, &buffer);

        // Update state
        buffer = new_buffer;
        mapped_ptr = new_mapped_ptr;
        is_mapped = true;

        // Update memory blocks
        auto last_block = --memory_blocks.end();
        if (last_block->is_free) {
            last_block->size += (new_size - capacity);
        } else {
            memory_blocks.emplace_back(capacity, new_size - capacity);
        }

        capacity = new_size;
    }

    void defragment() { merge_adjacent_blocks(); }

    void defragment_and_shrink() {
        defragment();

        if (memory_blocks.size() == 1 && memory_blocks.front().is_free) {
            resize(INITIAL_CAPACITY);
        } else if (memory_blocks.back().is_free) {
            u64 new_size = capacity - memory_blocks.back().size;
            resize(new_size);
        }
    }

    u64 get_capacity() const { return capacity; }
    u64 get_used_memory() const { return used_memory; }
    GLuint get_buffer() const { return buffer; }
    void bind() const {
        glBindBuffer(GL_ARRAY_BUFFER, buffer);
        glEnableVertexAttribArray(0);
        glVertexAttribIPointer(0, 2, GL_UNSIGNED_INT, sizeof(VertexData),
                               (void*)0);
    }

    // Direct access to mapped memory (use with caution)
    void* get_mapped_ptr() const { return mapped_ptr; }

   private:
    std::list<MemoryBlock>::iterator find_free_block(u64 size) {
        return std::find_if(memory_blocks.begin(), memory_blocks.end(),
                            [size](const MemoryBlock& block) {
                                return block.is_free && block.size >= size;
                            });
    }

    size_t split_and_allocate(std::list<MemoryBlock>::iterator it, u64 size) {
        u64 remaining_size = it->size - size;
        u64 offset = it->offset;

        if (remaining_size > 0) {
            it = memory_blocks.erase(it);
            it = memory_blocks.insert(it, MemoryBlock(offset, size, false));
            auto it1 = it;
            memory_blocks.insert(
                ++it1, MemoryBlock(offset + size, remaining_size, true));
        } else {
            it->is_free = false;
        }

        used_memory += size;
        allocations.push_back(it);
        return allocations.size() - 1;
    }

    void merge_adjacent_blocks() {
        auto current = memory_blocks.begin();
        while (current != memory_blocks.end()) {
            auto next = std::next(current);
            if (next != memory_blocks.end() && current->is_free &&
                next->is_free) {
                current->size += next->size;
                memory_blocks.erase(next);
            } else {
                ++current;
            }
        }
    }

    static u64 align_size(u64 size, u64 alignment) {
        return (size + alignment - 1) & ~(alignment - 1);
    }
};

}  // namespace GPU