#include "SkyRenderer.hpp"

#include <glm/gtc/constants.hpp>
#include <SDL3/SDL_timer.h>

#include "game/data_loaders/globals.h"

static constexpr float skyboxVertices[108] = {
        // positions
        -1.0f, 1.0f, -1.0f,
        -1.0f, -1.0f, -1.0f,
        1.0f, -1.0f, -1.0f,
        1.0f, -1.0f, -1.0f,
        1.0f, 1.0f, -1.0f,
        -1.0f, 1.0f, -1.0f,

        -1.0f, -1.0f, 1.0f,
        -1.0f, -1.0f, -1.0f,
        -1.0f, 1.0f, -1.0f,
        -1.0f, 1.0f, -1.0f,
        -1.0f, 1.0f, 1.0f,
        -1.0f, -1.0f, 1.0f,

        1.0f, -1.0f, -1.0f,
        1.0f, -1.0f, 1.0f,
        1.0f, 1.0f, 1.0f,
        1.0f, 1.0f, 1.0f,
        1.0f, 1.0f, -1.0f,
        1.0f, -1.0f, -1.0f,

        -1.0f, -1.0f, 1.0f,
        -1.0f, 1.0f, 1.0f,
        1.0f, 1.0f, 1.0f,
        1.0f, 1.0f, 1.0f,
        1.0f, -1.0f, 1.0f,
        -1.0f, -1.0f, 1.0f,

        -1.0f, 1.0f, -1.0f,
        1.0f, 1.0f, -1.0f,
        1.0f, 1.0f, 1.0f,
        1.0f, 1.0f, 1.0f,
        -1.0f, 1.0f, 1.0f,
        -1.0f, 1.0f, -1.0f,

        -1.0f, -1.0f, -1.0f,
        -1.0f, -1.0f, 1.0f,
        1.0f, -1.0f, -1.0f,
        1.0f, -1.0f, -1.0f,
        -1.0f, -1.0f, 1.0f,
        1.0f, -1.0f, 1.0f
    };

void SkyRenderer::init() {
    nightSkyRenderer.init("assets/sky/sky.png");
    nightSkyRenderer.addQuads(100, {44, 0}, 0.1, 2.0); //100 small stars
    nightSkyRenderer.addQuads(30, {40, 0}, 0.15, 4.0); //30 medium stars
    nightSkyRenderer.addQuads(7, {32, 0}, 0.2, 8.0); //7 big stars
    nightSkyRenderer.addQuad({0, 32}, 3, 32.0, {-100, 0, 0}); //moon

    sunRenderer.init("assets/sky/sky.png");
    sunRenderer.addQuad({0, 0}, 6, 32.0, {100, 0, 0}); //sun

    skyboxShader = new Shader("shaders/skybox/vert.glsl", "shaders/skybox/frag.glsl");
    glGenVertexArrays(1, &skyVAO);
    glBindVertexArray(skyVAO);
    glGenBuffers(1, &skyVBO);
    glBindBuffer(GL_ARRAY_BUFFER, skyVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(skyboxVertices), skyboxVertices,GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
    startTime = SDL_GetTicks();
}

void SkyRenderer::renderSkybox(const glm::mat4& view, const glm::mat4& proj, const Camera& camera) const {
    if (!skyboxShader) return;
    float currentTime = SDL_GetTicks();
    float elapsed = currentTime - startTime; //ms

    constexpr float cycleDuration = 60.f * 1000; //60s
    float t = (fmod(elapsed / cycleDuration, 1.f)); // [0..1]

    // Calculate sun/moon position based on t
    float angle = t * M_PI * 2; // full rotation over cycle

    glm::vec3 sunDir;

    float sunX = cos(angle);
    float sunY = sin(angle);
    sunDir = glm::normalize(glm::vec3(sunX, sunY, 0.0f));

    glDepthMask(GL_FALSE);
    glDisable(GL_CULL_FACE);
    skyboxShader->use();

    skyboxShader->setFloat("uTime", t);
    skyboxShader->setVec3("uSunDirection", sunDir);
    skyboxShader->setMat4("uView", view);
    skyboxShader->setMat4("uProjection", proj);

    glBindVertexArray(skyVAO);
    glDrawArrays(GL_TRIANGLES, 0, 36);

    nightSkyRenderer.render(view, proj, camera, angle, std::max(0.0f, -sinf(angle)));
    sunRenderer.render(view, proj, camera, angle, 1);

    glDepthMask(GL_TRUE);
    glEnable(GL_CULL_FACE);
}

SkyRenderer::~SkyRenderer() {
    glDeleteVertexArrays(1, &skyVAO);
    glDeleteBuffers(1, &skyVBO);
    delete skyboxShader;
}
