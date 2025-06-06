#pragma once

#include <cmath>
#include <functional>

#include <glm/glm.hpp>

#include "BlockType.h"

class ChunkData {
    static short getIndex(const glm::vec3& pos) {
        return pos.x + pos.y * SIZE + pos.z * SIZE * SIZE;
    }

public:
    static constexpr int SIZE = 32;
    static_assert((SIZE & (SIZE - 1)) == 0, "SIZE must be a power of 2");
    static constexpr int DEPTH = static_cast<int>(std::log2(SIZE));

    std::vector<BlockType> blocks;

    ChunkData() : blocks(SIZE * SIZE * SIZE, BlockType::AIR) {}

    void changeBlock(const glm::ivec3& pos, BlockType data) {
        blocks[getIndex(pos)] = data;
    }

    void deleteBlock(const glm::ivec3& pos) {
        blocks[getIndex(pos)] = BlockType::AIR;
    }

    static void deleteBlock(std::vector<BlockType>::iterator it) {
        *it = BlockType::AIR;
    }

    bool containsBlock(const glm::ivec3& pos) const {
        return blocks[getIndex(pos)] != BlockType::AIR;
    }

    BlockType& getBlock(const glm::ivec3& pos) {
        return blocks[getIndex(pos)];
    }

    auto begin() { return blocks.begin(); }
    auto end() { return blocks.end(); }
    auto begin() const { return blocks.begin(); }
    auto end() const { return blocks.end(); }

    template <typename Predicate>
    void deleteIf(Predicate pred) {
        static_assert(
            std::is_invocable_r_v<bool, Predicate, const BlockType>,
            "Predicate must be callable with (const BlockType) and return bool"
        );

        for (auto it = begin(); it != end();) {
            auto value = *it;
            if (pred(value)) {
                deleteBlock(it);
            }
            else {
                ++it;
            }
        }
    }
};
