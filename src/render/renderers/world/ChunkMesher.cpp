#include "ChunkMesher.h"

#include <glm/vec2.hpp>

std::array<bool, 6> ChunkMesher::checkAdjacentBlocks(
    World& world, const ChunkData& chunkData, const glm::ivec2& chunkPos,
    const glm::ivec3& blockPos) {
    std::array<bool, 6> res{};

    constexpr int extremePoses[6] = {
        0, Chunk::WIDTH - 1, 0, Chunk::DEPTH - 1, Chunk::HEIGHT - 1, 0};
    constexpr int affectedCoords[6] = {0, 0, 2, 2, 1, 1};
    constexpr glm::ivec3 minBlockPoses[6] = {
        glm::ivec3{Chunk::WIDTH - 1, 0, 0},
        glm::ivec3{-Chunk::WIDTH + 1, 0, 0},
        glm::ivec3{0, 0, Chunk::DEPTH - 1},
        glm::ivec3{0, 0, -Chunk::DEPTH + 1},
        glm::ivec3{0, -Chunk::HEIGHT + 1, 0},
        glm::ivec3{0, Chunk::HEIGHT - 1, 0}};
    constexpr glm::ivec2 chunkChanges[6] = {
        glm::ivec2{-1, 0}, glm::ivec2{1, 0}, glm::ivec2{0, -1},
        glm::ivec2{0, 1},  glm::ivec2{0, 0}, glm::ivec2{0, 0}};

    // check x+ x- z+ z-
    for (int f = 0; f < 4; f++) {
        auto adjacentPos = blockPos;
        bool chunkChanged = false;
        if (adjacentPos[affectedCoords[f]] == extremePoses[f]) {
            adjacentPos += minBlockPoses[f];
            chunkChanged = true;
        }
        if (chunkChanged) {
            glm::ivec2 newChunkCoords = chunkPos + chunkChanges[f];
            Chunk* nextChunk =
                world.getChunk(newChunkCoords.x, newChunkCoords.y);
            if (!nextChunk ||
                !nextChunk->getBlocks().containsBlock(adjacentPos)) {
                res[f] = true;
            }
        } else {
            advanceInDirection(static_cast<Facing>(f), adjacentPos);
            if (!chunkData.containsBlock(adjacentPos)) res[f] = true;
        }
    }

    // check y+ and y-
    for (int f = 4; f < 6; f++) {
        auto adjacentPos = blockPos;
        if (adjacentPos[affectedCoords[f]] == extremePoses[f]) {
            res[f] = true;
            continue;
        }
        advanceInDirection(static_cast<Facing>(f), adjacentPos);
        if (!chunkData.containsBlock(adjacentPos)) res[f] = true;
    }

    return res;
}

glm::ivec3 posFromHash32(short hash) {
    int x = hash & 0x1F;
    int z = (hash >> 5) & 0x1F;
    int y = (hash >> 10) & 0x1F;
    return {x, y, z};
}

short hashFromPos(const glm::ivec3& pos) { return ChunkData::getIndex(pos); }

void ChunkMesher::greedyMesh() {
    // Clear previous data
    for (auto& faces : greedChunkFaces)
        for (auto& dir : faces) dir.clear();

    // Define plane axes and fixed axis for each facing
    static constexpr std::tuple<int, int, int> axisMap[6] = {
        {2, 1, 0},  // LEFT:   plane axes Z(axis1) and Y(axis2), fixed X
        {2, 1, 0},  // RIGHT:  plane axes Z(axis1) and Y(axis2), fixed X
        {0, 1, 2},  // FRONT:  plane axes X(axis1) and Y(axis2), fixed Z
        {0, 1, 2},  // BACK:   plane axes X(axis1) and Y(axis2), fixed Z
        {0, 2, 1},  // TOP:    plane axes X(axis1) and Z(axis2), fixed Y
        {0, 2, 1}   // BOTTOM: plane axes X(axis1) and Z(axis2), fixed Y
    };

    for (int subChunk = 0; subChunk < Chunk::SUB_COUNT; subChunk++) {
        for (int facing = 0; facing < 6; facing++) {
            // Extract axes configuration
            const auto& axes = axisMap[facing];
            const int axis1 = std::get<0>(axes);      // First plane axis
            const int axis2 = std::get<1>(axes);      // Second plane axis
            const int fixedAxis = std::get<2>(axes);  // Fixed axis

            // Iterate through all possible fixed axis positions
            for (int fixedVal = 0; fixedVal < Chunk::WIDTH; fixedVal++) {
                for (auto&& i : processed) i = false;

                for (int a2 = 0; a2 < Chunk::WIDTH; a2++) {
                    for (int a1 = 0; a1 < Chunk::WIDTH; a1++) {
                        // Create position vector with fixed axis value
                        glm::ivec3 pos(0);
                        pos[axis1] = a1;
                        pos[axis2] = a2;
                        pos[fixedAxis] = fixedVal;

                        const short hashPos = hashFromPos(pos);
                        const size_t idx = a1 + a2 * Chunk::WIDTH;

                        // Skip if already processed or no face
                        if (processed[idx] ||
                            !chunkFaces[subChunk][facing].contains(hashPos))
                            continue;

                        const unsigned layer =
                            chunkFaces[subChunk][facing][hashPos];
                        int width = 1;
                        int height = 1;

                        // Find maximum width (along axis1)
                        for (int w = a1 + 1; w < Chunk::WIDTH; w++) {
                            glm::ivec3 testPos = pos;
                            testPos[axis1] = w;
                            const short testHash = hashFromPos(testPos);
                            const size_t testIdx = w + a2 * Chunk::WIDTH;

                            if (processed[testIdx] ||
                                !chunkFaces[subChunk][facing].contains(
                                    testHash) ||
                                chunkFaces[subChunk][facing][testHash] !=
                                    layer) {
                                break;
                            }
                            width++;
                        }

                        // Find maximum height (along axis2)
                        bool heightValid = true;
                        for (int h = a2 + 1; h < Chunk::WIDTH; h++) {
                            for (int w = a1; w < a1 + width; w++) {
                                glm::ivec3 testPos = pos;
                                testPos[axis1] = w;
                                testPos[axis2] = h;
                                const short testHash = hashFromPos(testPos);
                                const size_t testIdx = w + h * Chunk::WIDTH;

                                if (processed[testIdx] ||
                                    !chunkFaces[subChunk][facing].contains(
                                        testHash) ||
                                    chunkFaces[subChunk][facing][testHash] !=
                                        layer) {
                                    heightValid = false;
                                    break;
                                }
                            }
                            if (!heightValid) break;
                            height++;
                        }

                        // Mark entire rectangle as processed
                        for (int h = a2; h < a2 + height; h++) {
                            for (int w = a1; w < a1 + width; w++) {
                                processed[w + h * Chunk::WIDTH] = true;
                            }
                        }

                        // Create position with actual fixed value
                        glm::ivec3 actualPos(0);
                        actualPos[axis1] = a1;
                        actualPos[axis2] = a2;
                        actualPos[fixedAxis] = fixedVal;

                        // Calculate quad size
                        glm::ivec3 size(1);
                        size[axis1] = width;
                        size[axis2] = height;

                        // Add merged face
                        greedChunkFaces[subChunk][facing].emplace_back(
                            CubeModel::getFace(
                                static_cast<Facing>(facing), actualPos, layer,
                                size, glm::ivec2{size[axis1], size[axis2]}));
                    }
                }
            }
        }
    }
}
void ChunkMesher::generateChunkMesh(const glm::ivec2& chunkPos,
                                    GPU::MappedChunkBuffer& pool) {
    size_t total_size = 0;

    std::array<std::array<GPU::MappedChunkBuffer::GPUBufferView, 6>,
               Chunk::SUB_COUNT>
        subChunks{};

    for (int subChunkY = 0; subChunkY < Chunk::SUB_COUNT; subChunkY++) {
        for (int facing = 0; facing < 6; facing++) {
            subChunks[subChunkY][facing].size =
                greedChunkFaces[subChunkY][facing].size();
            subChunks[subChunkY][facing].offset = total_size;
            total_size += greedChunkFaces[subChunkY][facing].size();
        }
    }

    const size_t chunkID = Chunk::getId(chunkPos.x, chunkPos.y);

    auto gpuBuffer = pool.getAllocation(chunkID);
    if (gpuBuffer.is_free)
        gpuBuffer = pool.allocate(chunkID, total_size * sizeof(FaceMesh));

    pool.resizeAllocation(chunkID, total_size * sizeof(FaceMesh));

    for (int subChunk = 0; subChunk < Chunk::SUB_COUNT; subChunk++) {
        for (int facing = 0; facing < 6; facing++) {
            pool.write(chunkID, greedChunkFaces[subChunk][facing].data(),
                       subChunks[subChunk][facing].size * sizeof(FaceMesh),
                       subChunks[subChunk][facing].offset * sizeof(FaceMesh));
        }
    }

    pool.chunkViewData[chunkID] = subChunks;

    pool.sync();
}

void ChunkMesher::generateChunkMeshData(World& world,
                                        const glm::ivec2& chunkPos) {
    // clear all previous data
    for (auto& subChunk : chunkFaces)
        for (auto& dir : subChunk) dir.clear();

    Chunk* chunk = world.getChunk(chunkPos.x, chunkPos.y);
    if (!chunk) return;
    const ChunkData& chunkData = chunk->getBlocks();

    for (int y = 0; y < Chunk::HEIGHT; y++) {
        int subChunkY = y / Chunk::SUB_HEIGHT;
        for (int z = 0; z < Chunk::DEPTH; z++) {
            for (int x = 0; x < Chunk::WIDTH; x++) {
                if (!chunkData.containsBlock({x, y, z})) continue;
                const int layer = 0;
                glm::ivec3 blockPos{x, y, z};

                std::array<bool, 6> res =
                    checkAdjacentBlocks(world, chunkData, chunkPos, blockPos);

                // block pos in subchunk for correct mesh generation
                blockPos.y %= Chunk::SUB_HEIGHT;

                for (int f = 0; f < 6; f++) {
                    if (res[f])
                        chunkFaces[subChunkY][f][chunkData.getIndex(blockPos)] =
                            layer;
                }
            }
        }
    }
}

// Greedy meshing for a 2D plane
void greedyMeshPlane(
    const std::function<BlockType(int, int)>& sample,
    const std::function<void(int, int, int, int, BlockType)>& addQuad,
    int width, int height) {
    std::vector<bool> processed(width * height, false);

    for (int y = 0; y < height; y++) {
        for (int x = 0; x < width; x++) {
            if (processed[x + y * width]) continue;

            BlockType current = sample(x, y);
            if (current == BlockType::AIR) continue;

            // Find run width
            int runWidth = 1;
            while (x + runWidth < width) {
                BlockType next = sample(x + runWidth, y);
                if (next != current || processed[(x + runWidth) + y * width])
                    break;
                runWidth++;
            }

            // Find run height
            int runHeight = 1;
            bool heightValid = true;
            while (y + runHeight < height && heightValid) {
                for (int dx = 0; dx < runWidth; dx++) {
                    BlockType next = sample(x + dx, y + runHeight);
                    if (next != current ||
                        processed[(x + dx) + (y + runHeight) * width]) {
                        heightValid = false;
                        break;
                    }
                }
                if (heightValid) runHeight++;
            }

            // Mark processed
            for (int dy = 0; dy < runHeight; dy++) {
                for (int dx = 0; dx < runWidth; dx++) {
                    processed[(x + dx) + (y + dy) * width] = true;
                }
            }

            // Add quad
            addQuad(x, y, runWidth, runHeight, current);
        }
    }
}

void ChunkMesher::update(World& world, const glm::ivec2& chunkPos,
                         GPU::MappedChunkBuffer& pool) {
    generateChunkMeshData(world, chunkPos);
    greedyMesh();
    generateChunkMesh(chunkPos, pool);
}
