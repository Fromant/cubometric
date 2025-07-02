#ifndef CUBEMODEL_H
#define CUBEMODEL_H
#include <array>
#include <glm/vec2.hpp>

#include "FaceInstance.h"
#include "game/world/EFacing.h"

class CubeModel {
    constexpr static std::array<FaceInstance, 6> cubeFaces{
            FaceInstance{
                // Left face
                std::array<Vertex, 6>{
                    Vertex{0, 0, 0, 0, 0},
                    Vertex{0, 0, 1, 1, 0},
                    Vertex{0, 1, 1, 1, 1},
                    Vertex{0, 1, 1, 1, 1},
                    Vertex{0, 1, 0, 0, 1},
                    Vertex{0, 0, 0, 0, 0}
                }
            },
            FaceInstance{
                // Right face
                std::array<Vertex, 6>{
                    Vertex{1, 0, 1, 0, 0},
                    Vertex{1, 0, 0, 1, 0},
                    Vertex{1, 1, 0, 1, 1},
                    Vertex{1, 1, 0, 1, 1},
                    Vertex{1, 1, 1, 0, 1},
                    Vertex{1, 0, 1, 0, 0}
                }
            },
            FaceInstance{
                // Front face
                std::array<Vertex, 6>{
                    Vertex{1, 0, 0, 0, 0},
                    Vertex{0, 0, 0, 1, 0},
                    Vertex{0, 1, 0, 1, 1},
                    Vertex{0, 1, 0, 1, 1},
                    Vertex{1, 1, 0, 0, 1},
                    Vertex{1, 0, 0, 0, 0}
                }
            },
            FaceInstance{
                //back face
                std::array<Vertex, 6>{
                    Vertex{0, 0, 1, 0, 0},
                    Vertex{1, 0, 1, 1, 0},
                    Vertex{1, 1, 1, 1, 1},
                    Vertex{1, 1, 1, 1, 1},
                    Vertex{0, 1, 1, 0, 1},
                    Vertex{0, 0, 1, 0, 0}
                }
            },
            FaceInstance{
                // Top face
                std::array<Vertex, 6>{
                    Vertex{0, 1, 1, 0, 0},
                    Vertex{1, 1, 1, 1, 0},
                    Vertex{1, 1, 0, 1, 1},
                    Vertex{1, 1, 0, 1, 1},
                    Vertex{0, 1, 0, 0, 1},
                    Vertex{0, 1, 1, 0, 0}
                }
            },
            FaceInstance{
                // Bottom face
                std::array<Vertex, 6>{
                    Vertex{0, 0, 0, 0, 0},
                    Vertex{1, 0, 0, 1, 0},
                    Vertex{1, 0, 1, 1, 1},
                    Vertex{1, 0, 1, 1, 1},
                    Vertex{0, 0, 1, 0, 1},
                    Vertex{0, 0, 0, 0, 0}
                }
            }
        };

public:
    static constexpr FaceInstance getFace(const Facing f, const glm::ivec3& pos, const int layer,
                                          const glm::ivec3& scale = {1, 1, 1}) {
        FaceInstance tr = cubeFaces[f];

        for (auto& vertex : tr.vertices) {
            // Keep local position; chunk offset is applied in shader
            int x = vertex.getX() * scale.x + pos.x;
            int y = vertex.getY() * scale.y + pos.y;
            int z = vertex.getZ() * scale.z + pos.z;
            // No addition of pos.x/pos.y/pos.z here
            // Preserve texture bits (0x3000) and local position
            vertex = Vertex(x, y, z, vertex.getTexU(), vertex.getTexV(), layer);
        }
        return tr;
    }
};


#endif //CUBEMODEL_H
