#ifndef VERTEXRENDERER_H
#define VERTEXRENDERER_H
#include <vector>

#include "FaceInstance.h"
#include "../../utils/Shader.h"

namespace render {
    class BlockFaceRenderer {
        Shader *shader = nullptr;
        GLuint vao = 0, vbo = 0;
        size_t numVertices = 0;

        GLuint texture = 0;

    public:
        BlockFaceRenderer() = default;

        void Init();

        void updateVertices(const std::vector<FaceInstance> &faces);

        void Render(const glm::mat4 &view, const glm::mat4 &proj) const;

        size_t getNumVertices() const { return numVertices; }

        ~BlockFaceRenderer();
    };
} // render

#endif //VERTEXRENDERER_H
