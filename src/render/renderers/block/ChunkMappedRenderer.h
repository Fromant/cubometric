#ifndef CHUNKMAPPEDRENDERER_H
#define CHUNKMAPPEDRENDERER_H

#include "../../buffers/GPUBuffer.h"
#include "../../utils/Shader.h"

class ChunkMappedRenderer {
     Shader *shader = nullptr;

    GLuint VAO = 0;
    GLuint texture = 0;
public:
    void Init();

    void Render(const glm::mat4& view, const glm::mat4& proj, GPUBuffer* buffer, int numVertices);
};



#endif //CHUNKMAPPEDRENDERER_H
