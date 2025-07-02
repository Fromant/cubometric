#ifndef SKYRENDERER_HPP
#define SKYRENDERER_HPP
#include "QuadRenderer.hpp"
#include "render/utils/Shader.h"

class SkyRenderer {
    Shader* skyboxShader = nullptr;
    GLuint skyVAO = 0;
    GLuint skyVBO = 0;

    QuadRenderer nightSkyRenderer;
    QuadRenderer sunRenderer;

    float startTime = 0;

public:
    void renderSkybox(const glm::mat4& view, const glm::mat4& proj, const Camera& camera) const;
    void init();
    ~SkyRenderer();
};

#endif //SKYRENDERER_HPP
