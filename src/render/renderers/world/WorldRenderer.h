#ifndef WORLDRENDERER_H
#define WORLDRENDERER_H

#include "../../../game/world/World.h"
#include "../../buffers/MappedBufferPool.h"
#include "../../utils/Shader.h"
#include "../../Camera.h"

class WorldRenderer {
    MappedBufferPool bufferPool{VIEW_DISTANCE * VIEW_DISTANCE * 4};

    Shader* shader = nullptr;

    GLuint VAO = 0;
    GLuint texture = 0;

    void renderChunk(const glm::mat4& view, const glm::mat4& proj, GPUBuffer* buffer,
                     int numVertices, const glm::ivec3& coords) const;

public:
    static constexpr int VIEW_DISTANCE = 24; //internal const for now. 12 is right what my pc can handle

    int render(World& w, const Camera& c);

    void init();

    MappedBufferPool& getBufferPool() { return bufferPool; }
};

#endif //WORLDRENDERER_H
