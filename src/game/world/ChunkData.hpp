#pragma once

#include <functional>
#include <iostream>

#include <glm/glm.hpp>

#include "BlockType.h"

class ChunkData {
public:
    static size_t getIndex(const glm::ivec3& pos) {
        return pos.x + pos.y * WIDTH * DEPTH + pos.z * WIDTH;
    }


    static constexpr int WIDTH = 32;
    static constexpr int HEIGHT = 256;
    static constexpr int DEPTH = 32;

    std::vector<BlockType> blocks;

    ChunkData() : blocks(WIDTH * HEIGHT * DEPTH, BlockType::AIR) {}

    void changeBlock(const glm::ivec3& pos, BlockType data) {
        blocks.at(getIndex(pos)) = data;
        // blocks[getIndex(pos)] = data;
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

    const BlockType& getBlock(const glm::ivec3& pos) const {
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
