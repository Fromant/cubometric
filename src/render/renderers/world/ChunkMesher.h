#ifndef CHUNKMESHER_H
#define CHUNKMESHER_H
#include "../../../game/world/Chunk.h"
#include "../../buffers/MappedBufferPool.h"
#include "../../../render/renderers/block/CubeModel.h"
#include "../../../game/data_loaders/globals.h"

class ChunkMesher {
public:
    static void update(Chunk& chunk, MappedBufferPool& pool) {
        auto buffer = std::vector<FaceInstance>{};
        buffer.reserve(1000);
        const auto& blocks = chunk.getBlocks();

        const int xCoord = chunk.xCoord;
        const int yCoord = chunk.yCoord;
        const int zCoord = chunk.zCoord;

        for (int y = 0; y < Chunk::HEIGHT; y++) {
            for (int z = 0; z < Chunk::DEPTH; z++) {
                for (int x = 0; x < Chunk::WIDTH; x++) {
                    if (blocks[x + z * Chunk::DEPTH + y * Chunk::WIDTH * Chunk::DEPTH] == BlockType::AIR) continue;
                    //not air, add to mesh

                    const auto& data = textureManager.getTextureInfo("../assets/textures/blocks/dirt.png");
                    const glm::vec3 pos(x + xCoord * Chunk::WIDTH, y + yCoord * Chunk::HEIGHT,
                                        z + zCoord * Chunk::DEPTH);

                    //check z+ (west) (left)
                    if (z == (Chunk::DEPTH - 1) || blocks[x + (z + 1) * Chunk::DEPTH + y * Chunk::DEPTH * Chunk::WIDTH]
                        == BlockType::AIR) {
                        //if air, add side to mesh
                        buffer.emplace_back(CubeModel::getFace(WEST, pos, glm::vec2(data.uMin, data.vMin)));
                    }
                    //check z- (east) (right)
                    if (z == 0 || blocks[x + (z - 1) * Chunk::DEPTH + y * Chunk::DEPTH * Chunk::WIDTH] ==
                        BlockType::AIR) {
                        //if air, add side to mesh
                        buffer.emplace_back(CubeModel::getFace(EAST, pos, glm::vec2(data.uMin, data.vMin)));
                    }
                    //check x+ (south) (back)
                    if (x == (Chunk::WIDTH - 1) || blocks[x + 1 + z * Chunk::DEPTH + y * Chunk::DEPTH * Chunk::WIDTH] ==
                        BlockType::AIR) {
                        //if air, add side to mesh
                        buffer.emplace_back(CubeModel::getFace(SOUTH, pos, glm::vec2(data.uMin, data.vMin)));
                    }
                    //check x- (north) (front)
                    if (x == 0 || blocks[x - 1 + z * Chunk::DEPTH + y * Chunk::DEPTH * Chunk::WIDTH] ==
                        BlockType::AIR) {
                        //if air, add side to mesh
                        buffer.emplace_back(CubeModel::getFace(NORTH, pos, glm::vec2(data.uMin, data.vMin)));
                    }
                    //check y+ (top)
                    if (y == (Chunk::HEIGHT - 1) || blocks[x + z * Chunk::DEPTH + (y + 1) * Chunk::DEPTH * Chunk::WIDTH]
                        == BlockType::AIR) {
                        //if air, add side to mesh
                        buffer.emplace_back(CubeModel::getFace(UP, pos, glm::vec2(data.uMin, data.vMin)));
                    }
                    //check y- (bottom)
                    if (y == 0 || blocks[x + z * Chunk::DEPTH + (y - 1) * Chunk::DEPTH * Chunk::WIDTH] ==
                        BlockType::AIR) {
                        //if air, add side to mesh
                        buffer.emplace_back(CubeModel::getFace(DOWN, pos, glm::vec2(data.uMin, data.vMin)));
                    }
                }
            }
        }
        auto* gpuBuffer = pool.getBuffer(chunk.getId());
        if (!gpuBuffer)
            gpuBuffer = pool.createBuffer(chunk.getId(), buffer.size() * sizeof(FaceInstance));
        gpuBuffer->write(buffer.data(), buffer.size() * sizeof(FaceInstance));
    }
};


#endif //CHUNKMESHER_H
