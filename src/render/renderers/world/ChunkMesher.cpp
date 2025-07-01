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


void ChunkMesher::update(World& world, const glm::ivec2& pos, MappedBufferPool& pool) {
    for (auto& b : buffer)
        for (auto& b1 : b)
            b1.clear();

    std::array<std::array<GPUBuffer::GPUBufferView, 6>, Chunk::SUB_COUNT> subChunks{};

    Chunk* chunk = world.getChunk(pos.x, pos.y);
    if (!chunk) return;
    const ChunkData& chunkData = chunk->getBlocks();

    size_t total_size = 0;

    for (int subChunkY = 0; subChunkY < Chunk::SUB_COUNT; subChunkY++) {
        for (int y = subChunkY * Chunk::SUB_HEIGHT; y < (subChunkY + 1) * Chunk::SUB_HEIGHT; y++) {
            for (int z = 0; z < Chunk::DEPTH; z++) {
                for (int x = 0; x < Chunk::WIDTH; x++) {
                    if (!chunkData.containsBlock({x, y, z})) continue;
                    //not air, add to mesh

                    // const int layer = textureManager.getTextureLayer("assets/textures/blocks/dirt.png");
                    const int layer = 0;
                    const glm::ivec3 pos{x, y & 0x1F, z};
                    //check z+ (west) (left)
                    if (z == Chunk::DEPTH - 1) {
                        if (auto nextChunk = world.getChunk(chunk->xCoord, chunk->zCoord + 1);
                            !nextChunk ||
                            !nextChunk->getBlocks().containsBlock({x, y, 0})) {
                            buffer[subChunkY][WEST].emplace_back(
                                CubeModel::getFace(WEST, pos, layer));
                        }
                    }
                    else if (!chunkData.containsBlock({x, y, z + 1})) {
                        //if air, add side to mesh
                        buffer[subChunkY][WEST].emplace_back(CubeModel::getFace(WEST, pos, layer));
                    }

                    //check z- (east) (right)
                    if (z == 0) {
                        if (auto nextChunk = world.getChunk(chunk->xCoord, chunk->zCoord - 1);
                            !nextChunk ||
                            !nextChunk->getBlocks().containsBlock({x, y, Chunk::DEPTH - 1})) {
                            buffer[subChunkY][EAST].emplace_back(
                                CubeModel::getFace(EAST, pos, layer));
                        }
                    }
                    else if (!chunkData.containsBlock({x, y, z - 1})) {
                        //if air, add side to mesh
                        buffer[subChunkY][EAST].emplace_back(CubeModel::getFace(EAST, pos, layer));
                    }

                    //check x+ (south) (back)
                    if (x == (Chunk::WIDTH - 1)) {
                        if (auto nextChunk = world.getChunk(chunk->xCoord + 1, chunk->zCoord);
                            !nextChunk ||
                            !nextChunk->getBlocks().containsBlock({0, y, z})) {
                            buffer[subChunkY][SOUTH].emplace_back(
                                CubeModel::getFace(SOUTH, pos, layer));
                        }
                    }
                    else if (!chunkData.containsBlock({x + 1, y, z})) {
                        //if air, add side to mesh
                        buffer[subChunkY][SOUTH].emplace_back(CubeModel::getFace(SOUTH, pos, layer));
                    }

                    //check x- (north) (front)
                    if (x == 0) {
                        if (auto nextChunk = world.getChunk(chunk->xCoord - 1, chunk->zCoord);
                            !nextChunk ||
                            !nextChunk->getBlocks().containsBlock({Chunk::WIDTH - 1, y, z})) {
                            buffer[subChunkY][NORTH].emplace_back(
                                CubeModel::getFace(NORTH, pos, layer));
                        }
                    }
                    else if (!chunkData.containsBlock({x - 1, y, z})) {
                        //if air, add side to mesh
                        buffer[subChunkY][NORTH].emplace_back(CubeModel::getFace(NORTH, pos, layer));
                    }

                    //check y+ (top)
                    if (y == Chunk::HEIGHT - 1) {
                        buffer[subChunkY][UP].emplace_back(CubeModel::getFace(UP, pos, layer));
                    }
                    else if (!chunkData.containsBlock({x, y + 1, z})) {
                        //if air, add side to mesh
                        buffer[subChunkY][UP].emplace_back(CubeModel::getFace(UP, pos, layer));
                    }

                    //check y- (bottom)
                    if (y == 0) {
                        buffer[subChunkY][DOWN].emplace_back(CubeModel::getFace(DOWN, pos, layer));
                    }
                    else if (!chunkData.containsBlock({x, z, y - 1})) {
                        //if air, add side to mesh
                        buffer[subChunkY][DOWN].emplace_back(CubeModel::getFace(DOWN, pos, layer));
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
