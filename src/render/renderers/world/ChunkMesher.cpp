#include "ChunkMesher.h"

#include <map>
#include <glm/vec2.hpp>

glm::ivec3 posFromHash32(short hash) {
    int x = hash & 0x1F;
    int z = (hash >> 5) & 0x1F;
    int y = (hash >> 10) & 0x1F;
    return {x, y, z};
}

short hashFromPos(const glm::ivec3& pos) {
    return ChunkData::getIndex(pos);
}

void ChunkMesher::greedyMesh(int scale) {
    // Clear previous data
    for (auto& faces : greedChunkFaces)
        for (auto& dir : faces) dir.clear();

    // Define plane axes and fixed axis for each facing
    static constexpr std::tuple<int, int, int> axisMap[6] = {
            {2, 1, 0}, // LEFT:   plane axes Z(axis1) and Y(axis2), fixed X
            {2, 1, 0}, // RIGHT:  plane axes Z(axis1) and Y(axis2), fixed X
            {0, 1, 2}, // FRONT:  plane axes X(axis1) and Y(axis2), fixed Z
            {0, 1, 2}, // BACK:   plane axes X(axis1) and Y(axis2), fixed Z
            {0, 2, 1}, // TOP:    plane axes X(axis1) and Z(axis2), fixed Y
            {0, 2, 1} // BOTTOM: plane axes X(axis1) and Z(axis2), fixed Y
        };

    for (int subChunk = 0; subChunk < Chunk::SUB_COUNT; subChunk++) {
        for (int facing = 0; facing < 6; facing++) {
            // Extract axes configuration
            const auto& axes = axisMap[facing];
            const int axis1 = std::get<0>(axes); // First plane axis
            const int axis2 = std::get<1>(axes); // Second plane axis
            const int fixedAxis = std::get<2>(axes); // Fixed axis

            // Iterate through all possible fixed axis positions
            for (int fixedVal = 0; fixedVal < Chunk::WIDTH; fixedVal++) {
                for (auto&& i : processed)
                    i = false;

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
                        if (processed[idx] || !chunkFaces[subChunk][facing].contains(hashPos))
                            continue;

                        const unsigned layer = chunkFaces[subChunk][facing][hashPos];
                        int width = 1;
                        int height = 1;

                        // Find maximum width (along axis1)
                        for (int w = a1 + 1; w < Chunk::WIDTH; w++) {
                            glm::ivec3 testPos = pos;
                            testPos[axis1] = w;
                            const short testHash = hashFromPos(testPos);
                            const size_t testIdx = w + a2 * Chunk::WIDTH;

                            if (processed[testIdx] ||
                                !chunkFaces[subChunk][facing].contains(testHash) ||
                                chunkFaces[subChunk][facing][testHash] != layer) {
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
                                    !chunkFaces[subChunk][facing].contains(testHash) ||
                                    chunkFaces[subChunk][facing][testHash] != layer) {
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
                                static_cast<Facing>(facing),
                                actualPos,
                                layer,
                                size,
                                glm::ivec2{size[axis1], size[axis2]}
                            )
                        );
                    }
                }
            }
        }
    }
}

void ChunkMesher::generateChunkMesh(const glm::ivec2& chunkPos, MappedBufferPool& pool, int scale) {
    size_t total_size = 0;

    std::array<std::array<GPUBuffer::GPUBufferView, 6>, Chunk::SUB_COUNT> subChunks{};

    for (int subChunkY = 0; subChunkY < Chunk::SUB_COUNT; subChunkY++) {
        for (int facing = 0; facing < 6; facing++) {
            subChunks[subChunkY][facing].size = greedChunkFaces[subChunkY][facing].size();
            subChunks[subChunkY][facing].offset = total_size;
            total_size += greedChunkFaces[subChunkY][facing].size();
        }
    }

    const size_t chunkID = Chunk::getId(chunkPos.x, chunkPos.y, scale);

    auto* gpuBuffer = pool.getBuffer(chunkID);
    if (!gpuBuffer)
        gpuBuffer = pool.createBuffer(chunkID, total_size * sizeof(FaceMesh));

    gpuBuffer->resize(total_size * sizeof(FaceMesh));
    for (int subChunk = 0; subChunk < Chunk::SUB_COUNT; subChunk++) {
        for (int facing = 0; facing < 6; facing++) {
            gpuBuffer->writeUnsynced(greedChunkFaces[subChunk][facing].data(),
                                     subChunks[subChunk][facing].size * sizeof(FaceMesh),
                                     subChunks[subChunk][facing].offset * sizeof(FaceMesh), subChunks);
        }
    }
    gpuBuffer->sync();
}

int hashFromPos(int x, int z) {
    return (x & 0xFFFF) | (z << 16);
}

int hashFromPos(const glm::ivec2& pos) {
    return (pos.x & 0xFFFF) | (pos.y << 16);
}

int mod(int a, int b) {
    int r = a % b;
    return r < 0 ? r + b : r;
}

glm::ivec2 getChunkCoords(const glm::ivec3& blockPos) {
    return {std::floor((float)blockPos.x / Chunk::WIDTH), std::floor((float)blockPos.z / Chunk::DEPTH)};
}

std::array<bool, 6> ChunkMesher::checkAdjacentBlocks(World& world, const ChunkData& chunkData,
                                                     const glm::ivec2& chunkPos, const glm::ivec3& blockPos,
                                                     int scale) {
    std::array<bool, 6> res{};

    //check x+ x- z+ z-
    for (int f = 0; f < 4; f++) {
        //world pos
        auto adjacentPos = blockPos;
        advanceInDirection(static_cast<Facing>(f), adjacentPos, scale);

        glm::ivec3 adjacentPosChunk = {
                mod(adjacentPos.x, Chunk::WIDTH), adjacentPos.y, mod(adjacentPos.z, Chunk::DEPTH)
            };

        glm::ivec2 newChunkCoords = getChunkCoords(adjacentPos);

        const ChunkData& chunkData1 = world.getChunk(newChunkCoords.x, newChunkCoords.y)->getBlocks();
        if (!chunkData1.containsBlock(adjacentPosChunk)) res[f] = true;
    }

    //check y+ and y-
    for (int f = 4; f < 6; f++) {
        auto adjacentPos = blockPos;
        advanceInDirection(static_cast<Facing>(f), adjacentPos, scale);
        if (adjacentPos.y < 0) {
            res[f] = true;
            continue;
        }
        if (adjacentPos.y > Chunk::HEIGHT) {
            res[f] = true;
            continue;
        }
        glm::ivec3 adjacentPosChunk = {
                mod(adjacentPos.x, Chunk::WIDTH), adjacentPos.y, mod(adjacentPos.z, Chunk::DEPTH)
            };
        if (!chunkData.containsBlock(adjacentPosChunk)) res[f] = true;
    }

    return res;
}

void ChunkMesher::generateChunkMeshData(World& world, const glm::ivec2& chunkPos, int scale) {
    // clear all previous data
    for (auto& subChunk : chunkFaces)
        for (auto& dir : subChunk) dir.clear();

    for (int y = 0; y < Chunk::HEIGHT; y += scale) {
        int subChunkY = y / Chunk::SUB_HEIGHT / scale; //scaled
        for (int z = chunkPos.y * Chunk::DEPTH;
             z < Chunk::DEPTH * scale + chunkPos.y * Chunk::DEPTH; z += scale) {
            for (int x = chunkPos.x * Chunk::WIDTH;
                 x < Chunk::WIDTH * scale + chunkPos.x * Chunk::WIDTH; x += scale) {
                //world pos
                glm::ivec3 blockPos = glm::ivec3{x, y, z};
                glm::ivec3 blockPosChunk = glm::ivec3{mod(x, Chunk::WIDTH), y, mod(z, Chunk::DEPTH)};

                //super sub chunk scaled pos
                glm::ivec2 blockSuperSubChunk = {x / scale / Chunk::WIDTH, z / scale / Chunk::DEPTH};

                //sub chunk pos
                glm::ivec2 subChunkPos = getChunkCoords(blockPos);

                const ChunkData& chunkData = world.getChunk(subChunkPos.x, subChunkPos.y)->getBlocks();
                if (!chunkData.containsBlock(blockPosChunk)) continue;

                const int layer = 0;

                std::array<bool, 6> res = checkAdjacentBlocks(world, chunkData, subChunkPos, blockPos, scale);

                //block pos in super subchunk mesh (32x32x32)
                blockPos /= scale;
                blockPos = {
                        mod(blockPos.x, Chunk::WIDTH),
                        mod(blockPos.y, Chunk::SUB_HEIGHT),
                        mod(blockPos.z, Chunk::DEPTH)
                    };

                for (int f = 0; f < 6; f++) {
                    if (res[f])
                        chunkFaces[subChunkY][f][ChunkData::getIndex(blockPos)] = layer;
                }
            }
        }
    }
}

// Greedy meshing for a 2D plane
void greedyMeshPlane(const std::function<BlockType(int, int)>& sample,
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
                if (next != current || processed[(x + runWidth) + y * width]) break;
                runWidth++;
            }

            // Find run height
            int runHeight = 1;
            bool heightValid = true;
            while (y + runHeight < height && heightValid) {
                for (int dx = 0; dx < runWidth; dx++) {
                    BlockType next = sample(x + dx, y + runHeight);
                    if (next != current || processed[(x + dx) + (y + runHeight) * width]) {
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

void ChunkMesher::update(World& world, const glm::ivec2& chunkPos, MappedBufferPool& pool, int scale) {
    generateChunkMeshData(world, chunkPos, scale);
    greedyMesh(scale);
    generateChunkMesh(chunkPos, pool, scale);
}
