#ifndef MAPPEDBUFFERPOOL_H
#define MAPPEDBUFFERPOOL_H

#include <glm/glm.hpp>
#include <unordered_map>

#include "GPUBuffer.h"

class MappedBufferPool {
public:

    explicit MappedBufferPool(int capacity = 0) {
        m_buffers.reserve(capacity);
    }

    GPUBuffer* getBuffer(int chunkId) {
        auto it = m_buffers.find(chunkId);
        if (it != m_buffers.end()) {
            return &it->second;
        }
        return nullptr;
    }

    GPUBuffer* createBuffer(int chunkId, size_t initialCapacity = 0) {
        auto [it, inserted] = m_buffers.try_emplace(chunkId, initialCapacity);
        return &it->second;
    }

    void deleteBuffer(int chunkId) {
        m_buffers.erase(chunkId);
    }

private:
    std::unordered_map<int, GPUBuffer> m_buffers;
};

#endif //MAPPEDBUFFERPOOL_H
