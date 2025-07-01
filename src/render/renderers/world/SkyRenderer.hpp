#ifndef SKYRENDERER_HPP
#define SKYRENDERER_HPP
#include "render/utils/Shader.h"

class SkyRenderer {
    Shader* skyboxShader = nullptr;
    GLuint skyVAO = 0;
    GLuint skyVBO = 0;

    //Do not glDeleteTextures()
    GLuint texture = 0;

    float startTime = 0;

public:
    void renderSkybox(const glm::mat4& view, const glm::mat4& proj, const glm::vec3& pos) const;
    void init();
    ~SkyRenderer();
};

#endif //SKYRENDERER_HPP
