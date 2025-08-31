#pragma once
#include <glad/glad.h>
#include <SDL3/SDL.h>
#include <atomic>
#include <mutex>
#include <vector>
#include <list>
#include <unordered_map>
#include <algorithm>
#include <thread>
#include <chrono>
#include <iostream>
#include <cstring>

// Configuration - tweak these for your engine
constexpr size_t INITIAL_VBO_SIZE = 32 * 1024 * 1024;  // 32 MB
constexpr size_t MAX_VBO_SIZE = 512 * 1024 * 1024;     // 512 MB
constexpr size_t ALLOC_GRANULARITY = 256;              // Allocation alignment
constexpr size_t DEFRAG_THRESHOLD = 30;                // Defrag when >30% fragmentation
constexpr size_t MIN_DEFRAG_INTERVAL = 5000;           // ms between defrags

typedef unsigned long long u64;

namespace GPU {

class TripleBufferedAllocator {
public:
    struct MemoryBlock {
        u64 offset;
        u64 size;
        bool is_free;
        size_t allocation_id;

        MemoryBlock(u64 offset = 0, u64 size = 0, bool is_free = true, size_t allocation_id = SIZE_MAX)
            : offset(offset), size(size), is_free(is_free), allocation_id(allocation_id) {}
    };

    // Represents a single VBO buffer in the triple buffer system
    struct Buffer {
        GLuint id = 0;
        void* mapped_ptr = nullptr;
        u64 capacity = 0;
        u64 used_memory = 0;
        std::vector<MemoryBlock> memory_blocks;
        std::vector<MemoryBlock*> allocations;
        GLsync fence = nullptr;
        std::atomic<bool> valid{true};  // For RCU buffer versioning
        std::atomic<bool> flush_pending{false};

        Buffer() = default;

        // Delete copy operations
        Buffer(const Buffer&) = delete;
        Buffer& operator=(const Buffer&) = delete;

        // Define move operations
        Buffer(Buffer&& other) noexcept
            : id(other.id), mapped_ptr(other.mapped_ptr), capacity(other.capacity),
              used_memory(other.used_memory), memory_blocks(std::move(other.memory_blocks)),
              allocations(std::move(other.allocations)), fence(other.fence),
              valid(other.valid.load()), flush_pending(other.flush_pending.load()) {
            other.id = 0;
            other.mapped_ptr = nullptr;
            other.capacity = 0;
            other.used_memory = 0;
            other.fence = nullptr;
            other.valid = false;
            other.flush_pending = false;
        }

        Buffer& operator=(Buffer&& other) noexcept {
            if (this != &other) {
                // Clean up current state
                if (id && valid) {
                    glDeleteBuffers(1, &id);
                    valid = false;
                }
                if (fence) {
                    glDeleteSync(fence);
                    fence = nullptr;
                }

                // Move state from other
                id = other.id;
                mapped_ptr = other.mapped_ptr;
                capacity = other.capacity;
                used_memory = other.used_memory;
                memory_blocks = std::move(other.memory_blocks);
                allocations = std::move(other.allocations);
                fence = other.fence;
                valid = other.valid.load();
                flush_pending = other.flush_pending.load();

                // Reset other
                other.id = 0;
                other.mapped_ptr = nullptr;
                other.capacity = 0;
                other.used_memory = 0;
                other.fence = nullptr;
                other.valid = false;
                other.flush_pending = false;
            }
            return *this;
        }

        ~Buffer() {
            if (id && valid) {
                glDeleteBuffers(1, &id);
                valid = false;
            }
            if (fence) {
                glDeleteSync(fence);
                fence = nullptr;
            }
        }
    };

    // Represents an allocation across the triple buffer system
    struct Allocation {
        Buffer* buffer = nullptr;
        int buffer_index = -1;  // Track which buffer in the triple buffer
        size_t block_index = SIZE_MAX;
        u64 offset = 0;
        u64 size = 0;
    };

    // ban copying
    TripleBufferedAllocator(const TripleBufferedAllocator&) = delete;
    TripleBufferedAllocator& operator=(const TripleBufferedAllocator&) = delete;

private:
    // Triple buffer state
    Buffer buffers[3];
    std::atomic<int> write_index{0};
    std::atomic<int> flush_index{0};

    // Synchronization
    std::mutex alloc_mutex;
    std::mutex resize_mutex;
    std::atomic<bool> resize_in_progress{false};

    // RCU (Read-Copy-Update) for safe resizing
    std::vector<Buffer*> pending_deletes;
    std::atomic<bool> buffers_swapping{false};

    // Defragmentation state
    std::atomic<bool> defrag_in_progress{false};
    std::atomic<u64> last_defrag_time{0};

    // Metrics for monitoring
    u64 fragmentation = 0;
    u64 total_allocations = 0;
    u64 total_deallocations = 0;

    // SDL window for context sharing (needed for worker threads)
    SDL_Window* window = nullptr;

public:
    explicit TripleBufferedAllocator(SDL_Window* win, u64 initial_capacity = INITIAL_VBO_SIZE)
        : buffers{}, window(win) {
        // Initialize triple buffers
        for (int i = 0; i < 3; i++) {
            create_buffer(&buffers[i], initial_capacity);
        }

        // Initialize memory blocks (free space tracking)
        for (int i = 0; i < 3; i++) {
            buffers[i].memory_blocks.emplace_back(0, initial_capacity, true, SIZE_MAX);
            buffers[i].allocations.reserve(1024);
        }

        // Setup shared context for worker threads
        SDL_GL_SetAttribute(SDL_GL_SHARE_WITH_CURRENT_CONTEXT, 1);
    }

    ~TripleBufferedAllocator() {
        // Cleanup all buffers
        for (int i = 0; i < 3; i++) {
            buffers[i].valid = false;
        }

        // Delete pending buffers
        for (Buffer* buf : pending_deletes) {
            delete buf;
        }
    }

    // Worker threads: Allocate space for mesh data
    Allocation alloc(u64 size) {
        // Align to granularity
        size = align_size(size, ALLOC_GRANULARITY);

        // Try current write buffer first
        int current_index = write_index.load(std::memory_order_acquire);
        Buffer* current_buf = &buffers[current_index];
        Allocation alloc = try_allocate(current_buf, current_index, size);

        if (alloc.block_index != SIZE_MAX) {
            return alloc;
        }

        // Current buffer full - try next buffer
        int next_index = (current_index + 1) % 3;
        Buffer* next_buf = &buffers[next_index];

        // Check if next buffer is available (not being flushed)
        if (!next_buf->flush_pending) {
            // Signal that we're switching buffers
            write_index.store(next_index, std::memory_order_release);

            // Mark current buffer as needing flush
            current_buf->flush_pending = true;

            // Try allocating in the new buffer
            alloc = try_allocate(&buffers[next_index], next_index, size);
            if (alloc.block_index != SIZE_MAX) {
                return alloc;
            }
        }

        // Both buffers full - need to resize or wait
        handle_full_buffers(size);

        // Try again with the current buffer
        current_index = write_index.load();
        return try_allocate(&buffers[current_index], current_index, size);
    }

    // Worker threads: Deallocate space when chunk is unloaded
    void dealloc(const Allocation& alloc) {
        if (!alloc.buffer || alloc.block_index == SIZE_MAX) return;

        std::lock_guard<std::mutex> lock(alloc_mutex);

        MemoryBlock* block = alloc.buffer->allocations[alloc.block_index];
        block->is_free = true;
        alloc.buffer->used_memory -= block->size;

        // Merge adjacent free blocks
        merge_adjacent_blocks(alloc.buffer);

        // Update fragmentation metrics
        update_fragmentation(alloc.buffer);
    }

    // Worker threads: Write mesh data directly to mapped memory
    static void write(const void* data, u64 size, const Allocation& alloc) {
        if (!alloc.buffer || !data || size == 0) return;

        char* dest = static_cast<char*>(alloc.buffer->mapped_ptr) + alloc.offset;
        std::memcpy(dest, data, size);
    }

    // Render thread: Process flushes for completed buffers
    void process_flushes() {
        for (int i = 0; i < 3; i++) {
            Buffer* buf = &buffers[i];
            if (buf->flush_pending) {
                // Explicit flush (safer than relying on COHERENT)
                glFlushMappedBufferRange(buf->id, 0, buf->capacity);

                // Insert fence for safe buffer reuse
                if (buf->fence) {
                    glDeleteSync(buf->fence);
                }
                buf->fence = glFenceSync(GL_SYNC_GPU_COMMANDS_COMPLETE, 0);

                buf->flush_pending = false;
            }
        }
    }

    // Render thread: Check if old buffers can be deleted
    void cleanup_old_buffers() {
        std::vector<Buffer*> to_delete;
        {
            std::lock_guard<std::mutex> lock(resize_mutex);
            to_delete.swap(pending_deletes);
        }

        for (Buffer* buf : to_delete) {
            if (!buf->valid) continue;

            // Check if GPU is done with this buffer
            if (buf->fence) {
                GLenum result = glClientWaitSync(buf->fence, GL_SYNC_FLUSH_COMMANDS_BIT, 100000); // 0.1ms
                if (result == GL_TIMEOUT_EXPIRED) {
                    // Re-queue for later cleanup
                    std::lock_guard<std::mutex> lock(resize_mutex);
                    pending_deletes.push_back(buf);
                    continue;
                }
                glDeleteSync(buf->fence);
                buf->fence = nullptr;
            }

            // Safe to delete
            buf->valid = false;
            delete buf;
        }
    }

    // Manual defragmentation (call from render thread)
    void defragment() {
        u64 now = SDL_GetTicks();
        if (defrag_in_progress || (now - last_defrag_time) < MIN_DEFRAG_INTERVAL) {
            return;
        }

        // Only defrag if fragmentation is high
        if (fragmentation < DEFRAG_THRESHOLD) {
            return;
        }

        std::lock_guard<std::mutex> lock(alloc_mutex);
        defrag_in_progress = true;

        // Create temporary buffer
        Buffer* temp_buf = new Buffer();
        create_buffer(temp_buf, get_total_capacity());

        // Copy all active allocations to new buffer
        size_t offset = 0;
        std::vector<Allocation> active_allocs;

        for (int i = 0; i < 3; i++) {
            Buffer* src_buf = &buffers[i];
            for (size_t j = 0; j < src_buf->allocations.size(); j++) {
                MemoryBlock* block = src_buf->allocations[j];
                if (!block->is_free) {
                    // Copy data
                    char* src = static_cast<char*>(src_buf->mapped_ptr) + block->offset;
                    char* dst = static_cast<char*>(temp_buf->mapped_ptr) + offset;
                    std::memcpy(dst, src, block->size);

                    // Record new allocation
                    active_allocs.push_back({temp_buf, i, j, offset, block->size});
                    offset += block->size;
                }
            }
        }

        // Update all allocations to point to new buffer
        for (const auto& alloc : active_allocs) {
            MemoryBlock* block = &buffers[alloc.buffer_index].memory_blocks[alloc.block_index];
            block->offset = alloc.offset;
            // No need to update block->buffer since we're moving to a new system
        }

        // RCU: Swap to new buffer system
        Buffer* old_buffers[3];
        for (int i = 0; i < 3; i++) {
            old_buffers[i] = new Buffer(std::move(buffers[i]));
            buffers[i] = std::move(*temp_buf);  // Move the temporary buffer state
        }

        // Queue old buffers for deletion
        {
            std::lock_guard<std::mutex> lock(resize_mutex);
            for (int i = 0; i < 3; i++) {
                pending_deletes.push_back(old_buffers[i]);
            }
        }

        // Reset defrag state
        fragmentation = 0;
        last_defrag_time = now;
        defrag_in_progress = false;

        std::cout << "Defragmentation complete. Fragmentation: 0%" << std::endl;
    }

    // Utility functions
    u64 get_total_capacity() const {
        u64 total = 0;
        for (int i = 0; i < 3; i++) {
            total += buffers[i].capacity;
        }
        return total;
    }

    u64 get_total_used_memory() const {
        u64 total = 0;
        for (int i = 0; i < 3; i++) {
            total += buffers[i].used_memory;
        }
        return total;
    }

    u64 get_fragmentation() const {
        return fragmentation;
    }

    void bind() const {
        // Bind the current write buffer for rendering
        glBindBuffer(GL_ARRAY_BUFFER, buffers[write_index.load()].id);
    }

    GLuint get_buffer() const {
        return buffers[write_index.load()].id;
    }

    // Direct access to mapped memory (for debugging)
    void* get_mapped_ptr(int buffer_index) const {
        return buffers[buffer_index].mapped_ptr;
    }

private:
    static void create_buffer(Buffer* buf, u64 capacity) {
        glCreateBuffers(1, &buf->id);
        glNamedBufferStorage(buf->id, capacity, nullptr,
            GL_MAP_WRITE_BIT |
            GL_MAP_PERSISTENT_BIT |
            GL_DYNAMIC_STORAGE_BIT
        );

        buf->mapped_ptr = glMapNamedBufferRange(buf->id, 0, capacity,
            GL_MAP_WRITE_BIT |
            GL_MAP_PERSISTENT_BIT |
            GL_MAP_FLUSH_EXPLICIT_BIT
        );

        buf->capacity = capacity;
        buf->used_memory = 0;
    }

    Allocation try_allocate(Buffer* buf, int buffer_index, u64 size) {
        std::lock_guard<std::mutex> lock(alloc_mutex);

        // Find free block
        auto it = std::ranges::find_if(buf->memory_blocks,
                                       [size](const MemoryBlock& block) {
                                           return block.is_free && block.size >= size;
                                       });

        if (it == buf->memory_blocks.end()) {
            return Allocation{};
        }

        // Split block if needed
        u64 remaining_size = it->size - size;
        u64 offset = it->offset;

        if (remaining_size > 0) {
            // Create new free block
            buf->memory_blocks.insert(
                std::next(it),
                MemoryBlock(offset + size, remaining_size, true, SIZE_MAX)
            );

            // Update current block
            it->size = size;
            it->is_free = false;
            it->allocation_id = buf->allocations.size();
        } else {
            it->is_free = false;
            it->allocation_id = buf->allocations.size();
        }

        // Record allocation
        MemoryBlock* block_ptr = &(*it);
        buf->allocations.push_back(block_ptr);
        buf->used_memory += size;

        // Update fragmentation metrics
        update_fragmentation(buf);

        // Return allocation info
        return Allocation{
            buf,
            buffer_index,
            block_ptr->allocation_id,
            block_ptr->offset,
            block_ptr->size
        };
    }

    void handle_full_buffers(u64 size) {
        if (resize_in_progress) {
            // Wait briefly for resize to complete
            std::this_thread::sleep_for(std::chrono::microseconds(100));
            return;
        }

        // Try to acquire resize lock without blocking
        if (resize_mutex.try_lock()) {
            if (!resize_in_progress) {
                resize_in_progress = true;
                resize_mutex.unlock();

                // Perform resize in background
                std::thread(&TripleBufferedAllocator::resize_background, this).detach();
            } else {
                resize_mutex.unlock();
            }
        }
    }

    void resize_background() {
        // Create new buffer triple with larger capacity
        u64 new_capacity = std::min(
            buffers[0].capacity * 2,
            MAX_VBO_SIZE
        );

        if (new_capacity <= buffers[0].capacity) {
            resize_in_progress = false;
            return;
        }

        // Create new buffers in worker context
        SDL_GLContext worker_context = SDL_GL_CreateContext(window);
        SDL_GL_MakeCurrent(window, worker_context);

        Buffer new_buffers[3];
        for (int i = 0; i < 3; i++) {
            create_buffer(&new_buffers[i], new_capacity);

            // Copy active allocations
            for (size_t j = 0; j < buffers[i].allocations.size(); j++) {
                MemoryBlock* block = buffers[i].allocations[j];
                if (!block->is_free) {
                    char* src = static_cast<char*>(buffers[i].mapped_ptr) + block->offset;
                    char* dst = static_cast<char*>(new_buffers[i].mapped_ptr) + block->offset;
                    std::memcpy(dst, src, block->size);
                }
            }
        }

        SDL_GL_MakeCurrent(window, nullptr);
        SDL_GL_DestroyContext(worker_context);

        // RCU: Swap to new buffers
        buffers_swapping = true;

        Buffer* old_buffers[3];
        for (int i = 0; i < 3; i++) {
            old_buffers[i] = new Buffer(std::move(buffers[i]));
            buffers[i] = std::move(new_buffers[i]);
        }

        // Queue old buffers for deletion
        {
            std::lock_guard<std::mutex> lock(resize_mutex);
            for (int i = 0; i < 3; i++) {
                pending_deletes.push_back(old_buffers[i]);
            }
        }

        std::cout << "Resize complete. New capacity: " << new_capacity << " bytes" << std::endl;

        // Reset state
        resize_in_progress = false;
        buffers_swapping = false;
    }

    static void merge_adjacent_blocks(Buffer* buf) {
        auto current = buf->memory_blocks.begin();
        while (current != buf->memory_blocks.end()) {
            auto next = std::next(current);
            if (next != buf->memory_blocks.end() && current->is_free && next->is_free) {
                current->size += next->size;
                buf->memory_blocks.erase(next);
            } else {
                ++current;
            }
        }
    }

    void update_fragmentation(const Buffer* buf) {
        // Calculate fragmentation: free space that's not contiguous
        u64 free_space = buf->capacity - buf->used_memory;
        if (free_space == 0) {
            fragmentation = 0;
            return;
        }

        // Find the largest contiguous free block
        u64 largest_free_block = 0;
        for (const auto& block : buf->memory_blocks) {
            if (block.is_free && block.size > largest_free_block) {
                largest_free_block = block.size;
            }
        }

        // Fragmentation % = 100 - (largest_free_block / total_free_space * 100)
        fragmentation = 100 - (largest_free_block * 100 / free_space);
    }

    static u64 align_size(u64 size, u64 alignment) {
        return (size + alignment - 1) & ~(alignment - 1);
    }
};

// Thread-safe wrapper for TripleBufferedAllocator
class ThreadSafeMappedChunkBuffer {
public:
    explicit ThreadSafeMappedChunkBuffer(SDL_Window* window, int capacity = 0)
        : allocator(window, capacity) {
        chunk_allocs.reserve(1024);
    }

    TripleBufferedAllocator::Allocation getAllocation(size_t chunkId) {
        auto it = chunk_allocs.find(chunkId);
        if (it != chunk_allocs.end()) {
            return it->second;
        }
        return TripleBufferedAllocator::Allocation{};
    }

    TripleBufferedAllocator::Allocation allocate(size_t id, size_t capacity = 0) {
        auto [it, inserted] = chunk_allocs.try_emplace(id, allocator.alloc(capacity));
        return it->second;
    }

    void deallocate(size_t id) {
        auto it = chunk_allocs.find(id);
        if (it == chunk_allocs.end()) return;

        allocator.dealloc(it->second);
        chunk_allocs.erase(it);
    }

    bool containsAllocation(size_t id) const {
        return chunk_allocs.contains(id);
    }

    void resizeAllocation(size_t id, size_t new_size) {
        auto it = chunk_allocs.find(id);
        if (it != chunk_allocs.end()) {
            // Deallocate old allocation
            allocator.dealloc(it->second);

            // Allocate new space
            it->second = allocator.alloc(new_size);
        }
    }

    void sync() {
        allocator.process_flushes();
        allocator.cleanup_old_buffers();
    }

    void bind() const {
        allocator.bind();
    }

    u64 get_capacity() const {
        return allocator.get_total_capacity();
    }

    u64 get_used_memory() const {
        return allocator.get_total_used_memory();
    }

    GLuint get_buffer() const {
        return allocator.get_buffer();
    }

    void write(size_t id, const void* data, size_t size, size_t offset = 0) {
        auto it = chunk_allocs.find(id);
        if (it != chunk_allocs.end()) {
            const auto& alloc = it->second;
            if (!alloc.buffer) return;

            if (size + offset > alloc.size)
                throw std::runtime_error("write exceeds allocation limits");

            allocator.write(data, size, alloc);
        }
    }

    struct GPUBufferView {
        size_t offset = 0;
        size_t size = 0;
    };
    typedef std::array<std::array<GPUBufferView, 6>, 16> ChunkBufferView;

    std::unordered_map<u64, ChunkBufferView> chunkViewData;

    // Manual defragmentation
    void defragment() {
        allocator.defragment();
    }

    // Get fragmentation percentage
    u64 get_fragmentation() const {
        return allocator.get_fragmentation();
    }

private:
    std::unordered_map<size_t, TripleBufferedAllocator::Allocation> chunk_allocs;
    TripleBufferedAllocator allocator;
};

}  // namespace GPU