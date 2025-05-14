#ifndef CUBEMODEL_H
#define CUBEMODEL_H
#include <array>

#include "FaceInstance.h"
#include "../../../game/world/EFacing.h"
#include "../../../game/data_loaders/TextureManager.h"

class CubeModel {
    static inline std::array<FaceInstance, 6> cubeFaces{
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
    static FaceInstance getFace(const Facing f, const glm::vec3& pos, const glm::vec2& uvMap) {
        FaceInstance tr = cubeFaces[f];

        for (auto& vertex : tr.vertices) {
            auto& position = vertex.data;
            // Keep local position; chunk offset is applied in shader
            int x = (position & 0x1F) + pos.x;
            int y = ((position >> 5) & 0x1F) + pos.y;
            int z = ((position >> 10) & 0x1F) + pos.z;
            // No addition of pos.x/pos.y/pos.z here
            // Preserve texture bits (0x3000) and local position
            position = (position & 0x18000) | x | (y << 5) | (z << 10);
        }
        return tr;
    }
};


#endif //CUBEMODEL_H
