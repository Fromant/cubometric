#ifndef CHUNKMESHER_H
#define CHUNKMESHER_H
#include <glm/vec2.hpp>

#include "../../../game/world/Chunk.h"
#include "../../../game/world/World.h"
#include "../../buffers/MappedBufferPool.h"
#include "../../../render/renderers/block/CubeModel.h"
#include "../../../game/data_loaders/globals.h"

class ChunkMesher {
    static inline std::array<std::vector<FaceInstance>, 6> buffer{};
    static inline std::vector<FaceInstance> total;

public:
    static void update(const World& world, Chunk& chunk, MappedBufferPool& pool) {
        for (auto& b : buffer) b.clear();
        const auto& blocks = chunk.getBlocks();

        for (int y = 0; y < Chunk::HEIGHT; y++) {
            for (int z = 0; z < Chunk::DEPTH; z++) {
                for (int x = 0; x < Chunk::WIDTH; x++) {
                    if (blocks[x + z * Chunk::DEPTH + y * Chunk::WIDTH * Chunk::DEPTH] == BlockType::AIR) continue;
                    //not air, add to mesh

                    const auto& data = textureManager.getTextureInfo("../assets/textures/blocks/dirt.png");
                    const glm::vec3 pos(x, y, z);

                    //check z+ (west) (left)
                    if (z == Chunk::DEPTH - 1) {
                        if (auto nextChunk = world.getChunk(chunk.xCoord, chunk.yCoord, chunk.zCoord + 1);
                            !nextChunk ||
                            nextChunk->getBlocks()[x + y * Chunk::DEPTH * Chunk::WIDTH] == BlockType::AIR) {
                            buffer[WEST].emplace_back(
                                CubeModel::getFace(WEST, pos, glm::vec2(data.uMin, data.vMin)));
                        }
                    }
                    else if (blocks[x + (z + 1) * Chunk::DEPTH + y * Chunk::DEPTH * Chunk::WIDTH]
                        == BlockType::AIR) {
                        //if air, add side to mesh
                        buffer[WEST].emplace_back(CubeModel::getFace(WEST, pos, glm::vec2(data.uMin, data.vMin)));
                    }

                    //check z- (east) (right)
                    if (z == 0) {
                        if (auto nextChunk = world.getChunk(chunk.xCoord, chunk.yCoord, chunk.zCoord - 1);
                            !nextChunk ||
                            nextChunk->getBlocks()[x + (Chunk::DEPTH - 1) * Chunk::WIDTH + y * Chunk::DEPTH *
                                Chunk::WIDTH] == BlockType::AIR) {
                            buffer[EAST].emplace_back(
                                CubeModel::getFace(EAST, pos, glm::vec2(data.uMin, data.vMin)));
                        }
                    }
                    else if (blocks[x + (z - 1) * Chunk::DEPTH + y * Chunk::DEPTH * Chunk::WIDTH] ==
                        BlockType::AIR) {
                        //if air, add side to mesh
                        buffer[EAST].emplace_back(CubeModel::getFace(EAST, pos, glm::vec2(data.uMin, data.vMin)));
                    }

                    //check x+ (south) (back)
                    if (x == (Chunk::WIDTH - 1)) {
                        if (auto nextChunk = world.getChunk(chunk.xCoord + 1, chunk.yCoord, chunk.zCoord);
                            !nextChunk ||
                            nextChunk->getBlocks()[z * Chunk::WIDTH + y * Chunk::DEPTH *
                                Chunk::WIDTH] == BlockType::AIR) {
                            buffer[SOUTH].emplace_back(
                                CubeModel::getFace(SOUTH, pos, glm::vec2(data.uMin, data.vMin)));
                        }
                    }
                    else if (blocks[x + 1 + z * Chunk::DEPTH + y * Chunk::DEPTH * Chunk::WIDTH] ==
                        BlockType::AIR) {
                        //if air, add side to mesh
                        buffer[SOUTH].emplace_back(CubeModel::getFace(SOUTH, pos, glm::vec2(data.uMin, data.vMin)));
                    }

                    //check x- (north) (front)
                    if (x == 0) {
                        if (auto nextChunk = world.getChunk(chunk.xCoord - 1, chunk.yCoord, chunk.zCoord);
                            !nextChunk ||
                            nextChunk->getBlocks()[Chunk::WIDTH - 1 + z * Chunk::WIDTH + y * Chunk::DEPTH *
                                Chunk::WIDTH] == BlockType::AIR) {
                            buffer[NORTH].emplace_back(
                                CubeModel::getFace(NORTH, pos, glm::vec2(data.uMin, data.vMin)));
                        }
                    }
                    else if (blocks[x - 1 + z * Chunk::DEPTH + y * Chunk::DEPTH * Chunk::WIDTH] ==
                        BlockType::AIR) {
                        //if air, add side to mesh
                        buffer[NORTH].emplace_back(CubeModel::getFace(NORTH, pos, glm::vec2(data.uMin, data.vMin)));
                    }

                    //check y+ (top)
                    if (y == Chunk::HEIGHT - 1) {
                        if (auto nextChunk = world.getChunk(chunk.xCoord, chunk.yCoord + 1, chunk.zCoord);
                            !nextChunk ||
                            nextChunk->getBlocks()[x + z * Chunk::WIDTH] == BlockType::AIR) {
                            buffer[UP].emplace_back(
                                CubeModel::getFace(UP, pos, glm::vec2(data.uMin, data.vMin)));
                        }
                    }
                    else if (blocks[x + z * Chunk::DEPTH + (y + 1) * Chunk::DEPTH * Chunk::WIDTH]
                        == BlockType::AIR) {
                        //if air, add side to mesh
                        buffer[UP].emplace_back(CubeModel::getFace(UP, pos, glm::vec2(data.uMin, data.vMin)));
                    }

                    //check y- (bottom)
                    if (y == 0) {
                        if (auto nextChunk = world.getChunk(chunk.xCoord, chunk.yCoord - 1, chunk.zCoord);
                            !nextChunk ||
                            nextChunk->getBlocks()[x + z * Chunk::WIDTH + (Chunk::HEIGHT - 1) * Chunk::WIDTH *
                                Chunk::DEPTH] == BlockType::AIR) {
                            buffer[DOWN].emplace_back(
                                CubeModel::getFace(DOWN, pos, glm::vec2(data.uMin, data.vMin)));
                        }
                    }
                    else if (blocks[x + z * Chunk::DEPTH + (y - 1) * Chunk::DEPTH * Chunk::WIDTH] ==
                        BlockType::AIR) {
                        //if air, add side to mesh
                        buffer[DOWN].emplace_back(CubeModel::getFace(DOWN, pos, glm::vec2(data.uMin, data.vMin)));
                    }
                }
            }
        }

        size_t totalSize = 0;
        for (int i = 0; i < 6; i++) {
            chunk.faceOffsets[i] = totalSize;
            totalSize += buffer[i].size();
            chunk.faceSizes[i] = buffer[i].size();
        }
        total.clear();
        total.reserve(totalSize);
        for (auto& b : buffer) {
            total.insert(total.end(), b.begin(), b.end());
        }


        const size_t chunkID = chunk.getId();
        auto* gpuBuffer = pool.getBuffer(chunkID);
        if (!gpuBuffer)
            gpuBuffer = pool.createBuffer(chunkID, totalSize * sizeof(FaceInstance));
        gpuBuffer->write(total.data(), totalSize * sizeof(FaceInstance));
    }
};


#endif //CHUNKMESHER_H
