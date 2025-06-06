#include "ChunkMesher.h"

#include <glm/vec2.hpp>

void ChunkMesher::update(World& world, Chunk& chunk, MappedBufferPool& pool) {
    for (auto& b : buffer) b.clear();
    const ChunkData& chunkData = chunk.getBlocks();

    for (int y = 0; y < Chunk::HEIGHT; y++) {
        for (int z = 0; z < Chunk::DEPTH; z++) {
            for (int x = 0; x < Chunk::WIDTH; x++) {
                if (!chunkData.containsBlock({x, y, z})) continue;
                //not air, add to mesh

                // const int layer = textureManager.getTextureLayer("assets/textures/blocks/dirt.png");
                const int layer = 0;
                const glm::ivec3 pos{x, y, z};
                //check z+ (west) (left)
                if (z == Chunk::DEPTH - 1) {
                    if (auto nextChunk = world.getChunk(chunk.xCoord, chunk.yCoord, chunk.zCoord + 1);
                        !nextChunk ||
                        !nextChunk->getBlocks().containsBlock({x, y, 0})) {
                        buffer[WEST].emplace_back(
                            CubeModel::getFace(WEST, pos, layer));
                    }
                }
                else if (!chunkData.containsBlock({x, y, z + 1})) {
                    //if air, add side to mesh
                    buffer[WEST].emplace_back(CubeModel::getFace(WEST, pos, layer));
                }

                //check z- (east) (right)
                if (z == 0) {
                    if (auto nextChunk = world.getChunk(chunk.xCoord, chunk.yCoord, chunk.zCoord - 1);
                        !nextChunk ||
                        !nextChunk->getBlocks().containsBlock({x, y, Chunk::DEPTH - 1})) {
                        buffer[EAST].emplace_back(
                            CubeModel::getFace(EAST, pos, layer));
                    }
                }
                else if (!chunkData.containsBlock({x, y, z - 1})) {
                    //if air, add side to mesh
                    buffer[EAST].emplace_back(CubeModel::getFace(EAST, pos, layer));
                }

                //check x+ (south) (back)
                if (x == (Chunk::WIDTH - 1)) {
                    if (auto nextChunk = world.getChunk(chunk.xCoord + 1, chunk.yCoord, chunk.zCoord);
                        !nextChunk ||
                        !nextChunk->getBlocks().containsBlock({0, y, z})) {
                        buffer[SOUTH].emplace_back(
                            CubeModel::getFace(SOUTH, pos, layer));
                    }
                }
                else if (!chunkData.containsBlock({x + 1, y, z})) {
                    //if air, add side to mesh
                    buffer[SOUTH].emplace_back(CubeModel::getFace(SOUTH, pos, layer));
                }

                //check x- (north) (front)
                if (x == 0) {
                    if (auto nextChunk = world.getChunk(chunk.xCoord - 1, chunk.yCoord, chunk.zCoord);
                        !nextChunk ||
                        !nextChunk->getBlocks().containsBlock({Chunk::WIDTH - 1, y, z})) {
                        buffer[NORTH].emplace_back(
                            CubeModel::getFace(NORTH, pos, layer));
                    }
                }
                else if (!chunkData.containsBlock({x - 1, y, z})) {
                    //if air, add side to mesh
                    buffer[NORTH].emplace_back(CubeModel::getFace(NORTH, pos, layer));
                }

                //check y+ (top)
                if (y == Chunk::HEIGHT - 1) {
                    if (auto nextChunk = world.getChunk(chunk.xCoord, chunk.yCoord + 1, chunk.zCoord);
                        !nextChunk ||
                        !nextChunk->getBlocks().containsBlock({x, 0, z})) {
                        buffer[UP].emplace_back(
                            CubeModel::getFace(UP, pos, layer));
                    }
                }
                else if (!chunkData.containsBlock({x, y + 1, z})) {
                    //if air, add side to mesh
                    buffer[UP].emplace_back(CubeModel::getFace(UP, pos, layer));
                }

                //check y- (bottom)
                if (y == 0) {
                    if (auto nextChunk = world.getChunk(chunk.xCoord, chunk.yCoord - 1, chunk.zCoord);
                        !nextChunk ||
                        !nextChunk->getBlocks().containsBlock({x, Chunk::HEIGHT - 1, z})) {
                        buffer[DOWN].emplace_back(
                            CubeModel::getFace(DOWN, pos, layer));
                    }
                }
                else if (!chunkData.containsBlock({x, z, y - 1})) {
                    //if air, add side to mesh
                    buffer[DOWN].emplace_back(CubeModel::getFace(DOWN, pos, layer));
                }
            }
        }
    }

    size_t totalFaces = 0;
    for (int i = 0; i < 6; i++) {
        chunk.faceOffsets[i] = totalFaces;
        totalFaces += buffer[i].size();
        chunk.faceCounts[i] = buffer[i].size();
    }
    const size_t chunkID = chunk.getId();
    auto* gpuBuffer = pool.getBuffer(chunkID);
    if (!gpuBuffer)
        gpuBuffer = pool.createBuffer(chunkID, totalFaces * sizeof(FaceInstance));
    for (int i = 0; i < 6; i++) {
        gpuBuffer->write(buffer[i].data(), buffer[i].size() * sizeof(FaceInstance),
                         chunk.faceOffsets[i] * sizeof(FaceInstance));
    }
}
