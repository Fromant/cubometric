#pragma once
#include "render/Camera.h"
#include "render/utils/Shader.h"

struct Quad {
    glm::vec3 position;
    float scale;
    float size;
    glm::vec2 uvStart;
};

class QuadRenderer {

    Shader* quadShader = nullptr;
    GLuint quadVAO = 0, instanceVBO = 0, quadVBO = 0;
    std::vector<Quad> quads;
    GLuint atlasTexture = 0;

public:
    void init(const std::string& atlas);

    void render(const glm::mat4& view, const glm::mat4& proj, const Camera& camera, float rotation, float opacity) const;

    void addQuads(int count, const glm::vec2& start, float scale, float size);
    void addQuad(const glm::vec2& start, float scale, float size, const glm::vec3& pos);

    ~QuadRenderer();
};
