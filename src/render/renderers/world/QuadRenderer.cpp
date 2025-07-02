#include "QuadRenderer.hpp"

#include <glm/ext/matrix_transform.hpp>

#include "game/data_loaders/globals.h"

void QuadRenderer::init(const std::string& atlas) {
    quadShader = new Shader("shaders/quad/vert.glsl", "shaders/quad/frag.glsl");
    quadShader->use();
    atlasTexture = textureManager.
        loadTexture2D(atlas, GL_RGBA, GL_UNSIGNED_BYTE, GL_NEAREST, GL_NEAREST);
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, atlasTexture);
    quadShader->setSampler("uAtlas", 1);

    // constexpr float quadVertices[] = {
    //         -1, -1, 1, 1, -1, 1, 1, 1, 1,
    //         -1, -1, 1, 1, 1, 1, -1, 1, 1
    //     };

    constexpr float quadVertices[] = {
            -1, -1, 0, 1, -1, 0, 1, 1, 0,
            -1, -1, 0, 1, 1, 0, -1, 1, 0
        };

    glGenVertexArrays(1, &quadVAO);
    glBindVertexArray(quadVAO);

    glGenBuffers(1, &quadVBO);
    glBindBuffer(GL_ARRAY_BUFFER, quadVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(quadVertices), quadVertices, GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, 0);

    glGenBuffers(1, &instanceVBO);

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
}


void QuadRenderer::addQuads(int count, const glm::vec2& uvStart, float scale, float size) {
    quads.reserve(count);
    for (int i = 0; i < count; ++i) {
        float theta = rand() * 2 * M_PI / RAND_MAX;
        float phi = acos(2.0f * rand() / RAND_MAX - 1.0f);
        const int radius = 64;
        quads.push_back({
                glm::vec3(sin(phi) * cos(theta) * radius, cos(phi) * radius, sin(phi) * sin(theta) * radius),
                scale, size / 64.0f, uvStart / 64.0f
            });
    }

    // Instance VBO
    glBindBuffer(GL_ARRAY_BUFFER, instanceVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(Quad) * quads.size(), quads.data(), GL_STATIC_DRAW);

    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Quad), (void*)0);
    glEnableVertexAttribArray(2);
    glVertexAttribPointer(2, 1, GL_FLOAT, GL_FALSE, sizeof(Quad), (void*)12);
    glEnableVertexAttribArray(3);
    glVertexAttribPointer(3, 1, GL_FLOAT, GL_FALSE, sizeof(Quad), (void*)16);
    glEnableVertexAttribArray(4);
    glVertexAttribPointer(4, 2, GL_FLOAT, GL_FALSE, sizeof(Quad), (void*)20);
    glVertexAttribDivisor(1, 1);
    glVertexAttribDivisor(2, 1);
    glVertexAttribDivisor(3, 1);
    glVertexAttribDivisor(4, 1);
}

void QuadRenderer::addQuad(const glm::vec2& start, float scale, float size, const glm::vec3& pos) {
    quads.push_back({
            pos,
            scale, size / 64.0f, start / 64.0f
        });

    // Instance VBO
    glBindBuffer(GL_ARRAY_BUFFER, instanceVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(Quad) * quads.size(), quads.data(), GL_STATIC_DRAW);

    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Quad), (void*)0);
    glEnableVertexAttribArray(2);
    glVertexAttribPointer(2, 1, GL_FLOAT, GL_FALSE, sizeof(Quad), (void*)12);
    glEnableVertexAttribArray(3);
    glVertexAttribPointer(3, 1, GL_FLOAT, GL_FALSE, sizeof(Quad), (void*)16);
    glEnableVertexAttribArray(4);
    glVertexAttribPointer(4, 2, GL_FLOAT, GL_FALSE, sizeof(Quad), (void*)20);
    glVertexAttribDivisor(1, 1);
    glVertexAttribDivisor(2, 1);
    glVertexAttribDivisor(3, 1);
    glVertexAttribDivisor(4, 1);
}


void QuadRenderer::render(const glm::mat4& view, const glm::mat4& proj, const Camera& camera,
                          float rotation, float opacity) const {
    // glDisable(GL_DEPTH_TEST);

    quadShader->use();

    glm::mat4 skyRotation = glm::rotate(glm::mat4(1.0f), rotation, {0, 0, 1});
    // glm::mat4 model = glm::mat4(1.0f);

    quadShader->setMat4("uMVP", proj * glm::mat4(glm::mat3(view)));
    quadShader->setMat4("uSkyRotation", skyRotation);
    quadShader->setVec3("uCameraRight", camera.getRight());
    quadShader->setVec3("uCameraUp", camera.getUp());
    quadShader->setVec3("uCameraPos", camera.getPosition());
    quadShader->setFloat("uOpacity", opacity);
    glBindTexture(GL_TEXTURE_2D, atlasTexture);

    // Single draw call for all celestial bodies
    glBindVertexArray(quadVAO);
    glDrawArraysInstanced(GL_TRIANGLES, 0, 6, quads.size());

    // glDisable(GL_BLEND);
    // glEnable(GL_DEPTH_TEST);
}

QuadRenderer::~QuadRenderer() {
    glDeleteBuffers(1, &instanceVBO);
    glDeleteBuffers(1, &quadVBO);
    glDeleteVertexArrays(1, &quadVAO);
    delete quadShader;
}
