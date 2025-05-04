#ifndef CUBEMODEL_H
#define CUBEMODEL_H
#include <array>

#include "FaceInstance.h"
#include "../../../game/world/EFacing.h"
#include "../../../game/data_loaders/TextureManager.h"

class CubeModel {
    static constexpr std::array<FaceInstance, 6> cubeFaces = {
        FaceInstance{
            //back face
            std::array<Vertex, 6>{
                Vertex{
                    glm::vec3(0.0f, 0.0f, 1.0f),
                    glm::vec2(0.0f, 0.0f)
                },
                Vertex{
                    glm::vec3(1.0f, 0.0f, 1.0f),
                    glm::vec2(1.0f, 0.0f),

                },
                Vertex{
                    glm::vec3(1.0f, 1.0f, 1.0f),
                    glm::vec2(1.0f, 1.0f),
                },
                Vertex{
                    glm::vec3(1.0f, 1.0f, 1.0f),
                    glm::vec2(1.0f, 1.0f),
                },
                Vertex{
                    glm::vec3(0.0f, 1.0f, 1.0f),
                    glm::vec2(0.0f, 1.0f),
                },
                Vertex{
                    glm::vec3(0.0f, 0.0f, 1.0f),
                    glm::vec2(0.0f, 0.0f)
                }
            }
        },
        FaceInstance{
            // Front face
            std::array<Vertex, 6>{
                Vertex{
                    glm::vec3(1.0f, 0.0f, 0.0f),
                    glm::vec2(0.0f, 0.0f)
                },
                Vertex{
                    glm::vec3(0.0f, 0.0f, 0.0f),
                    glm::vec2(1.0f, 0.0f)
                },
                Vertex{
                    glm::vec3(0.0f, 1.0f, 0.0f),
                    glm::vec2(1.0f, 1.0f)
                },
                Vertex{
                    glm::vec3(0.0f, 1.0f, 0.0f),
                    glm::vec2(1.0f, 1.0f)

                },
                Vertex{
                    glm::vec3(1.0f, 1.0f, 0.0f),
                    glm::vec2(0.0f, 1.0f)
                },
                Vertex{
                    glm::vec3(1.0f, 0.0f, 0.0f),
                    glm::vec2(0.0f, 0.0f)
                }
            }
        },
        FaceInstance{
            // Left face
            std::array<Vertex, 6>{
                Vertex{
                    glm::vec3(0.0f, 0.0f, 0.0f),
                    glm::vec2(0.0f, 0.0f)
                },
                Vertex{
                    glm::vec3(0.0f, 0.0f, 1.0f),
                    glm::vec2(1.0f, 0.0f)
                },
                Vertex{
                    glm::vec3(0.0f, 1.0f, 1.0f),
                    glm::vec2(1.0f, 1.0f),

                },
                Vertex{
                    glm::vec3(0.0f, 1.0f, 1.0f),
                    glm::vec2(1.0f, 1.0f)
                },
                Vertex{
                    glm::vec3(0.0f, 1.0f, 0.0f),
                    glm::vec2(0.0f, 1.0f)
                },
                Vertex{
                    glm::vec3(0.0f, 0.0f, 0.0f),
                    glm::vec2(0.0f, 0.0f)
                }
            }
        },
        FaceInstance{
            // Right face
            std::array<Vertex, 6>{
                Vertex{
                    glm::vec3(1.0f, 0.0f, 1.0f),
                    glm::vec2(0.0f, 0.0f)
                },
                Vertex{
                    glm::vec3(1.0f, 0.0f, 0.0f),
                    glm::vec2(1.0f, 0.0f)
                },
                Vertex{
                    glm::vec3(1.0f, 1.0f, 0.0f),
                    glm::vec2(1.0f, 1.0f),

                },
                Vertex{
                    glm::vec3(1.0f, 1.0f, 0.0f),
                    glm::vec2(1.0f, 1.0f)
                },
                Vertex{
                    glm::vec3(1.0f, 1.0f, 1.0f),
                    glm::vec2(0.0f, 1.0f)
                },
                Vertex{
                    glm::vec3(1.0f, 0.0f, 1.0f),
                    glm::vec2(0.0f, 0.0f)
                }
            }
        },
        FaceInstance{
            // Top face
            std::array<Vertex, 6>{
                Vertex{
                    glm::vec3(0.0f, 1.0f, 1.0f),
                    glm::vec2(0.0f, 0.0f)
                },
                Vertex{
                    glm::vec3(1.0f, 1.0f, 1.0f),
                    glm::vec2(1.0f, 0.0f)
                },
                Vertex{
                    glm::vec3(1.0f, 1.0f, 0.0f),
                    glm::vec2(1.0f, 1.0f),

                },
                Vertex{
                    glm::vec3(1.0f, 1.0f, 0.0f),
                    glm::vec2(1.0f, 1.0f)
                },
                Vertex{
                    glm::vec3(0.0f, 1.0f, 0.0f),
                    glm::vec2(0.0f, 1.0f)
                },
                Vertex{
                    glm::vec3(0.0f, 1.0f, 1.0f),
                    glm::vec2(0.0f, 0.0f)
                }
            }
        },
        FaceInstance{
            // Bottom face
            std::array<Vertex, 6>{
                Vertex{
                    glm::vec3(0.0f, 0.0f, 0.0f),
                    glm::vec2(0.0f, 0.0f)
                },
                Vertex{
                    glm::vec3(1.0f, 0.0f, 0.0f),
                    glm::vec2(1.0f, 0.0f)
                },
                Vertex{
                    glm::vec3(1.0f, 0.0f, 1.0f),
                    glm::vec2(1.0f, 1.0f),

                },
                Vertex{
                    glm::vec3(1.0f, 0.0f, 1.0f),
                    glm::vec2(1.0f, 1.0f)
                },
                Vertex{
                    glm::vec3(0.0f, 0.0f, 1.0f),
                    glm::vec2(0.0f, 1.0f)
                },
                Vertex{
                    glm::vec3(0.0f, 0.0f, 0.0f),
                    glm::vec2(0.0f, 0.0f)
                }
            }
        }
    };

public:
    static constexpr FaceInstance getFace(const Facing f, const glm::vec3 &pos, const glm::vec2 &uvMap) {
        FaceInstance tr = cubeFaces[f];

        for (auto &[position, texCoord]: tr.vertices) {
            position += pos;
            texCoord = texCoord * TextureManager::UV_STEP + uvMap;
        }
        return tr;
    }
};


#endif //CUBEMODEL_H
