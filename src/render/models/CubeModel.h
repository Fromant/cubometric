#ifndef CUBEMODEL_H
#define CUBEMODEL_H
#include <array>
#include <glm/vec2.hpp>

#include "FaceMesh.h"
#include "game/world/EFacing.h"

class CubeModel {
    constexpr static std::array<FaceMesh, 6> cubeFaces{
            FaceMesh{
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
            FaceMesh{
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
            FaceMesh{
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
            FaceMesh{
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
            FaceMesh{
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
            FaceMesh{
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
    static constexpr FaceMesh getFace(const Facing f, const glm::ivec3& pos, const unsigned int layer,
                                      const glm::ivec3& scale = {1, 1, 1}, const glm::ivec2& UVscale = {1,1}) {
        FaceMesh tr = cubeFaces[f];

        for (auto& vertex : tr.vertices) {
            // Keep local position; chunk offset is applied in shader
            unsigned int x = vertex.getX() * scale.x + pos.x;
            unsigned int y = vertex.getY() * scale.y + pos.y;
            unsigned int z = vertex.getZ() * scale.z + pos.z;

            unsigned int u = vertex.getTexU() * UVscale[0];
            unsigned int v = vertex.getTexV() * UVscale[1];
            // No addition of pos.x/pos.y/pos.z here
            // Preserve texture bits (0x3000) and local position
            vertex = Vertex(x, y, z, u, v, layer);
        }
        return tr;
    }
};


#endif //CUBEMODEL_H
