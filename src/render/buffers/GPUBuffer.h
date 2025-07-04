#pragma once

#include <algorithm>
#include <memory>
#include <glad/glad.h>

#include "game/world/EFacing.h"
#include "render/renderers/block/Vertex.h"

class GPUBuffer {
public:
    struct GPUBufferView {
        size_t offset = 0;
        size_t size = 0;
    };

    explicit GPUBuffer(size_t initialCapacity = 0) {
        if (initialCapacity > 0) resize(initialCapacity);
    }

    std::array<std::array<GPUBufferView, 6>, Chunk::HEIGHT / Chunk::WIDTH> subChunks{};

    ~GPUBuffer() {
        if (m_id) {
            glDeleteBuffers(1, &m_id);
        }
        if (m_fence) {
            glDeleteSync(m_fence);
        }
    }

    // Disallow copying
    GPUBuffer(const GPUBuffer&) = delete;
    GPUBuffer& operator=(const GPUBuffer&) = delete;

    // Move operations
    GPUBuffer(GPUBuffer&& other) noexcept
        : m_id(other.m_id), m_capacity(other.m_capacity),
          m_mapped(other.m_mapped), m_fence(other.m_fence), m_size(other.m_size) {
        other.m_id = 0;
        other.m_capacity = 0;
        other.m_size = 0;
        other.m_mapped = nullptr;
        other.m_fence = nullptr;
    }

    GPUBuffer& operator=(GPUBuffer&& other) noexcept {
        if (this != &other) {
            this->~GPUBuffer();
            new(this) GPUBuffer(std::move(other));
        }
        return *this;
    }

    const GPUBufferView& getView(size_t y, Facing f) const {
        return subChunks[f][y];
    }

    void bind() const {
        glBindBuffer(GL_ARRAY_BUFFER, m_id);
        glEnableVertexAttribArray(0);
        glVertexAttribIPointer(0, 2, GL_UNSIGNED_INT, sizeof(VertexData), (void*)0);
    }

    void write(const void* data, size_t size, size_t offset,
               const std::array<std::array<GPUBufferView, 6>, Chunk::SUB_COUNT>& subChunks) {
        this->subChunks = subChunks;
        if (offset + size > m_capacity) {
            resize(calculateGrowth(offset + size));
        }

        waitSync();
        memcpy(static_cast<char*>(m_mapped) + offset, data, size);
        m_size = size+offset;
        glBindBuffer(GL_ARRAY_BUFFER, m_id);
        glFlushMappedBufferRange(GL_ARRAY_BUFFER, static_cast<long long>(offset), static_cast<long long>(size));
    }

    void writeUnsynced(const void* data, size_t size, size_t offset,
                       const std::array<std::array<GPUBufferView, 6>, Chunk::SUB_COUNT>& subChunks) {
        this->subChunks = subChunks;
        if (offset + size > m_capacity) {
            resize(calculateGrowth(offset + size));
        }
        m_size = offset + size;
        memcpy(static_cast<char*>(m_mapped) + offset, data, size);
    }

    void sync() {
        waitSync();
        glBindBuffer(GL_ARRAY_BUFFER, m_id);
        glFlushMappedBufferRange(GL_ARRAY_BUFFER, 0, m_size);

    }

    void resize(size_t newCapacity) {
        if (newCapacity <= m_capacity) return;

        waitSync();
        newCapacity = calculateGrowth(newCapacity);

        GLuint newId;
        glGenBuffers(1, &newId);
        glBindBuffer(GL_ARRAY_BUFFER, newId);
        glBufferStorage(GL_ARRAY_BUFFER, newCapacity, nullptr,
                        GL_MAP_WRITE_BIT | GL_MAP_PERSISTENT_BIT | GL_DYNAMIC_STORAGE_BIT);

        void* newMapped = glMapBufferRange(GL_ARRAY_BUFFER, 0, newCapacity,
                                           GL_MAP_WRITE_BIT | GL_MAP_PERSISTENT_BIT | GL_MAP_FLUSH_EXPLICIT_BIT);

        if (m_id) {
            glDeleteBuffers(1, &m_id);
        }

        m_id = newId;
        m_capacity = newCapacity;
        m_mapped = newMapped;
    }

    void notifySubmitted() {
        waitSync(); // Cleanup previous fence
        m_fence = glFenceSync(GL_SYNC_GPU_COMMANDS_COMPLETE, 0);
    }

    GLuint id() const { return m_id; }
    size_t capacity() const { return m_capacity; }
    size_t size() const { return m_size; }
    void* mappedPtr() const { return m_mapped; }

private:
    void waitSync() {
        if (m_fence) {
            GLenum result = glClientWaitSync(m_fence, GL_SYNC_FLUSH_COMMANDS_BIT, GL_TIMEOUT_IGNORED);
            glDeleteSync(m_fence);
            m_fence = nullptr;
        }
    }

    size_t calculateGrowth(size_t newSize) const {
        constexpr size_t minGrowth = 1024; // 1KB minimum growth
        size_t newCapacity = std::max(m_capacity + minGrowth, newSize + 1);
        return newCapacity;
    }

    GLuint m_id = 0;
    size_t m_capacity = 0;
    size_t m_size = 0;
    void* m_mapped = nullptr;
    GLsync m_fence = nullptr;
};
