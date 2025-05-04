#ifndef CHUNK_H
#define CHUNK_H

#include <array>
#include <vector>
#include "../../render/renderers/block/FaceInstance.h"

#include "BlockType.h"

class Chunk {
public:
    static constexpr int WIDTH = 16;
    static constexpr int HEIGHT = 512;
    static constexpr int DEPTH = 16;

private:
    std::array<BlockType, DEPTH * WIDTH * HEIGHT> blocks{}; //bare 256kb chunk data.
    std::vector<FaceInstance> buffer{};
    int xCoord;
    int zCoord;

public:
    Chunk(int x, int z) : xCoord(x), zCoord(z) {
        for (int i = 0; i < WIDTH * DEPTH; i++) {
            blocks[i] = BlockType::DIRT;
        }
    };

    Chunk() = delete;

    void updateMesh();

    const std::vector<FaceInstance> &getFaces() const { return buffer; }
    void getFaces(std::vector<FaceInstance> &in) const { in.insert(in.end(), buffer.begin(), buffer.end()); }
};

#endif //CHUNK_H
