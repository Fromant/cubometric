#include "ChunkMesher.h"

#include <glm/vec2.hpp>

void ChunkMesher::updateLOD(World& world, const glm::ivec2& pos, MappedBufferPool& pool, int LOD) {
    for (auto& b : buffer)
        for (auto& b1 : b)
            b1.clear();

    std::array<std::array<GPUBuffer::GPUBufferView, 6>, Chunk::SUB_COUNT> subChunks{};

    LowDetailChunk* chunk = world.getLowDetailChunk(pos.x, pos.y, LOD);
    if (!chunk) return;

    const LowDetailChunkData& chunkData = chunk->getBlocks();

    size_t total_size = 0;

    const glm::ivec3 scale{1 << chunk->getLODLevel()};

    for (int subChunkY = 0; subChunkY < Chunk::SUB_COUNT; subChunkY++) {
        for (int y = subChunkY * chunkData.height / Chunk::SUB_COUNT; y < (subChunkY + 1) * chunkData.height /
             Chunk::SUB_COUNT; y++) {
            for (int z = 0; z < chunkData.depth; z++) {
                for (int x = 0; x < chunkData.width; x++) {
                    if (!chunkData.containsBlock({x, y, z})) continue;
                    //not air, add to mesh

                    // const int layer = textureManager.getTextureLayer("assets/textures/blocks/dirt.png");
                    const int layer = 0;
                    glm::ivec3 pos{x, y, z};
                    pos *= scale;
                    pos.y &= 0x1F;

                    //check z+ (west) (left)
                    if (z == chunkData.depth - 1) {
                        if (auto nextChunk = world.getLowDetailChunk(pos.x, pos.y + 1, std::max(LOD - 1, 1)); !nextChunk
                            ||
                            !nextChunk->getBlocks().containsBlock({x, y, 0})) {
                            buffer[subChunkY][WEST].emplace_back(
                                CubeModel::getFace(WEST, pos, layer, scale));
                        }
                    }
                    else if (!chunkData.containsBlock({x, y, z + 1})) {
                        //if air, add side to mesh
                        buffer[subChunkY][WEST].emplace_back(CubeModel::getFace(WEST, pos, layer, scale));
                        buffer[subChunkY][WEST].emplace_back(CubeModel::getFace(WEST, pos, layer, scale));
                    }

                    //check z- (east) (right)
                    if (z == 0) {
                        if (auto nextChunk = world.getLowDetailChunk(pos.x, pos.y - 1, std::max(LOD - 1, 1));
                            !nextChunk ||
                            !nextChunk->getBlocks().containsBlock({x, y, chunkData.depth - 1})) {
                            buffer[subChunkY][EAST].emplace_back(
                                CubeModel::getFace(EAST, pos, layer, scale));
                        }
                    }
                    else if (!chunkData.containsBlock({x, y, z - 1})) {
                        //if air, add side to mesh
                        buffer[subChunkY][EAST].emplace_back(CubeModel::getFace(EAST, pos, layer, scale));
                    }

                    //check x+ (south) (back)
                    if (x == (chunkData.width - 1)) {
                        if (auto nextChunk = world.getLowDetailChunk(pos.x + 1, pos.y, std::max(LOD - 1, 1));
                            !nextChunk ||
                            !nextChunk->getBlocks().containsBlock({0, y, z})) {
                            buffer[subChunkY][SOUTH].emplace_back(
                                CubeModel::getFace(SOUTH, pos, layer, scale));
                        }
                    }
                    else if (!chunkData.containsBlock({x + 1, y, z})) {
                        //if air, add side to mesh
                        buffer[subChunkY][SOUTH].emplace_back(CubeModel::getFace(SOUTH, pos, layer, scale));
                    }

                    //check x- (north) (front)
                    if (x == 0) {
                        if (auto nextChunk = world.getLowDetailChunk(pos.x - 1, pos.y, std::max(LOD - 1, 1));
                            !nextChunk ||
                            !nextChunk->getBlocks().containsBlock({chunkData.width - 1, y, z})) {
                            buffer[subChunkY][NORTH].emplace_back(
                                CubeModel::getFace(NORTH, pos, layer, scale));
                        }
                    }
                    else if (!chunkData.containsBlock({x - 1, y, z})) {
                        //if air, add side to mesh
                        buffer[subChunkY][NORTH].emplace_back(CubeModel::getFace(NORTH, pos, layer, scale));
                    }

                    //check y+ (top)
                    if (y == chunkData.height - 1) {
                        buffer[subChunkY][UP].emplace_back(CubeModel::getFace(UP, pos, layer, scale));
                    }
                    else if (!chunkData.containsBlock({x, y + 1, z})) {
                        //if air, add side to mesh
                        buffer[subChunkY][UP].emplace_back(CubeModel::getFace(UP, pos, layer, scale));
                    }

                    //check y- (bottom)
                    if (y == 0) {
                        buffer[subChunkY][DOWN].emplace_back(CubeModel::getFace(DOWN, pos, layer, scale));
                    }
                    else if (!chunkData.containsBlock({x, z, y - 1})) {
                        //if air, add side to mesh
                        buffer[subChunkY][DOWN].emplace_back(CubeModel::getFace(DOWN, pos, layer, scale));
                    }
                }
            }
        }
        for (int i = 0; i < 6; i++) {
            subChunks[subChunkY][i].size = buffer[subChunkY][i].size();
            subChunks[subChunkY][i].offset = total_size;
            total_size += buffer[subChunkY][i].size();
        }
    }

    const size_t chunkID = Chunk::getId(pos.x, pos.y);

    auto* gpuBuffer = pool.getBuffer(chunkID);
    if (!gpuBuffer)
        gpuBuffer = pool.createBuffer(chunkID, total_size * sizeof(FaceInstance));

    totalFaces.reserve(total_size);
    totalFaces.clear();
    for (const auto& subChunk : buffer) {
        for (const auto& dir : subChunk) {
            totalFaces.insert(totalFaces.end(), dir.begin(), dir.end());
        }
    }

    gpuBuffer->write(totalFaces.data(), totalFaces.size() * sizeof(FaceInstance), 0, subChunks);
}

std::array<bool, 6> ChunkMesher::checkAdjacentBlocks(World& world, const ChunkData& chunkData,
                                                     const glm::ivec2& chunkPos,
                                                     const glm::ivec3& blockPos) {
    std::array<bool, 6> res{};

    constexpr int extremePoses[6] = {0, Chunk::WIDTH - 1, 0, Chunk::DEPTH - 1, Chunk::HEIGHT - 1, 0};
    constexpr int affectedCoords[6] = {0, 0, 2, 2, 1, 1};
    constexpr glm::ivec3 minBlockPoses[6] = {
            glm::ivec3{Chunk::WIDTH - 1, 0, 0},
            glm::ivec3{-Chunk::WIDTH + 1, 0, 0},
            glm::ivec3{0, 0, Chunk::DEPTH - 1},
            glm::ivec3{0, 0, -Chunk::DEPTH + 1},
            glm::ivec3{0, -Chunk::HEIGHT + 1, 0},
            glm::ivec3{0, Chunk::HEIGHT - 1, 0}
        };
    constexpr glm::ivec2 chunkChanges[6] = {
            glm::ivec2{-1, 0},
            glm::ivec2{1, 0},
            glm::ivec2{0, -1},
            glm::ivec2{0, 1},
            glm::ivec2{0, 0},
            glm::ivec2{0, 0}
        };

    //check x+ x- z+ z-
    for (int f = 0; f < 4; f++) {
        auto adjacentPos = blockPos;
        bool chunkChanged = false;
        if (adjacentPos[affectedCoords[f]] == extremePoses[f]) {
            adjacentPos += minBlockPoses[f];
            chunkChanged = true;
        }
        if (chunkChanged) {
            glm::ivec2 newChunkCoords = chunkPos + chunkChanges[f];
            Chunk* nextChunk = world.getChunk(newChunkCoords.x, newChunkCoords.y);
            if (!nextChunk ||
                !nextChunk->getBlocks().containsBlock(adjacentPos)) {
                res[f] = true;
            }
        }
        else {
            advanceInDirection(static_cast<Facing>(f), adjacentPos);
            if (!chunkData.containsBlock(adjacentPos)) res[f] = true;
        }
    }

    //check y+ and y-
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

void ChunkMesher::update(World& world, const glm::ivec2& chunkPos, MappedBufferPool& pool) {
    for (auto& b : buffer)
        for (auto& b1 : b)
            b1.clear();

    std::array<std::array<GPUBuffer::GPUBufferView, 6>, Chunk::SUB_COUNT> subChunks{};

    Chunk* chunk = world.getChunk(chunkPos.x, chunkPos.y);
    if (!chunk) return;
    const ChunkData& chunkData = chunk->getBlocks();

    size_t total_size = 0;

    for (int y = 0; y < Chunk::HEIGHT; y++) {
        int subChunkY = y / Chunk::SUB_HEIGHT;
        for (int z = 0; z < Chunk::DEPTH; z++) {
            for (int x = 0; x < Chunk::WIDTH; x++) {
                if (!chunkData.containsBlock({x, y, z})) continue;
                const int layer = 0;
                glm::ivec3 blockPos{x, y, z};

                std::array<bool, 6> res = checkAdjacentBlocks(world, chunkData, chunkPos, blockPos);

                //block pos in subchunk for correct mesh generation
                blockPos.y %= Chunk::SUB_HEIGHT;

                for (int f = 0; f < 6; f++) {
                    if (res[f])
                        buffer[subChunkY][f].emplace_back(
                            CubeModel::getFace(static_cast<Facing>(f), blockPos, layer));
                }
            }
        }

        if (y % Chunk::SUB_HEIGHT == Chunk::SUB_HEIGHT - 1)
            for (int i = 0; i < 6; i++) {
                subChunks[subChunkY][i].size = buffer[subChunkY][i].size();
                subChunks[subChunkY][i].offset = total_size;
                total_size += buffer[subChunkY][i].size();
            }
    }

    const size_t chunkID = Chunk::getId(chunkPos.x, chunkPos.y);

    auto* gpuBuffer = pool.getBuffer(chunkID);
    if (!gpuBuffer)
        gpuBuffer = pool.createBuffer(chunkID, total_size * sizeof(FaceInstance));

    totalFaces.reserve(total_size);
    totalFaces.clear();
    for (const auto& subChunk : buffer) {
        for (const auto& dir : subChunk) {
            totalFaces.insert(totalFaces.end(), dir.begin(), dir.end());
        }
    }

    gpuBuffer->write(totalFaces.data(), totalFaces.size() * sizeof(FaceInstance), 0, subChunks);
}
