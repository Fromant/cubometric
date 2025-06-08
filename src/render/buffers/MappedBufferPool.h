#ifndef MAPPEDBUFFERPOOL_H
#define MAPPEDBUFFERPOOL_H

#include <unordered_map>

#include "GPUBuffer.h"

class MappedBufferPool {
public:
    explicit MappedBufferPool(int capacity = 0) {
        m_buffers.reserve(capacity);
    }

    GPUBuffer* getBuffer(size_t chunkId) {
        auto it = m_buffers.find(chunkId);
        if (it != m_buffers.end()) {
            return &it->second;
        }
        return nullptr;
    }

    GPUBuffer* createBuffer(size_t chunkId, size_t initialCapacity = 0) {
        auto [it, inserted] = m_buffers.try_emplace(chunkId, initialCapacity);
        return &it->second;
    }

    void deleteBuffer(size_t chunkId) {
        m_buffers.erase(chunkId);
    }

    bool containsBuffer(size_t chunkId) {
        return m_buffers.find(chunkId) != m_buffers.end();
    }

private:
    std::unordered_map<size_t, GPUBuffer> m_buffers;
};

#endif //MAPPEDBUFFERPOOL_H
