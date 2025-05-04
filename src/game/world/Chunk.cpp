#include "Chunk.h"

#include <glm/geometric.hpp>

#include "../../render/renderers/block/CubeModel.h"
#include "../data_loaders/globals.h"


void Chunk::updateMesh() {
    int y = 0;
    buffer.clear();
    // for (int y = 0; y < HEIGHT; y++) {
    for (int z = 0; z < DEPTH; z++) {
        for (int x = 0; x < WIDTH; x++) {
            if (blocks[x + z * DEPTH + y * WIDTH * DEPTH] == BlockType::AIR) continue;
            //not air, add to mesh

            const auto &data = textureManager.getTextureInfo("../assets/textures/blocks/dirt.png");
            const glm::vec3 pos(x + xCoord * WIDTH, y, z + zCoord * DEPTH);

            //check z+ (west) (left)
            if (z == (DEPTH - 1) || blocks[x + (z + 1) * DEPTH + y * DEPTH * WIDTH] == BlockType::AIR) {
                //if air, add side to mesh
                buffer.emplace_back(CubeModel::getFace(WEST, pos, glm::vec2(data.uMin, data.vMin)));
            }
            //check z- (east) (right)
            if (z == 0 || blocks[x + (z - 1) * DEPTH + y * DEPTH * WIDTH] == BlockType::AIR) {
                //if air, add side to mesh
                buffer.emplace_back(CubeModel::getFace(EAST, pos, glm::vec2(data.uMin, data.vMin)));
            }
            //check x+ (south) (back)
            if (x == (WIDTH - 1) || blocks[x + 1 + z * DEPTH + y * DEPTH * WIDTH] == BlockType::AIR) {
                //if air, add side to mesh
                buffer.emplace_back(CubeModel::getFace(SOUTH, pos, glm::vec2(data.uMin, data.vMin)));
            }
            //check x- (north) (front)
            if (x == 0 || blocks[x - 1 + z * DEPTH + y * DEPTH * WIDTH] == BlockType::AIR) {
                //if air, add side to mesh
                buffer.emplace_back(CubeModel::getFace(NORTH, pos, glm::vec2(data.uMin, data.vMin)));
            }
            //check y+ (top)
            if (y == (HEIGHT - 1) || blocks[x + z * DEPTH + (y + 1) * DEPTH * WIDTH] == BlockType::AIR) {
                //if air, add side to mesh
                buffer.emplace_back(CubeModel::getFace(UP, pos, glm::vec2(data.uMin, data.vMin)));
            }
            //check y- (bottom)
            if (y == 0 || blocks[x + z * DEPTH + (y - 1) * DEPTH * WIDTH] == BlockType::AIR) {
                //if air, add side to mesh
                buffer.emplace_back(CubeModel::getFace(DOWN, pos, glm::vec2(data.uMin, data.vMin)));
            }
        }
    }
    // }
}
