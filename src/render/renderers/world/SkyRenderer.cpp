#include "SkyRenderer.hpp"

#include <iostream>
#include <vector>
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
    skyboxShader = new Shader("shaders/skybox/vert.glsl", "shaders/skybox/frag.glsl");
    skyboxShader->use();
    texture = textureManager.loadTexture2D("assets/skybox/front.jpg", GL_RGB, GL_UNSIGNED_BYTE, GL_NEAREST, GL_NEAREST);
    // texture = textureManager.loadTexture2D("assets/textures/sky/sky.png", GL_RGBA, GL_UNSIGNED_BYTE, GL_NEAREST, GL_NEAREST);
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, texture);
    skyboxShader->setSampler("sky", 1);

    glGenVertexArrays(1, &skyVAO);
    glBindVertexArray(skyVAO);
    glGenBuffers(1, &skyVBO);
    glBindBuffer(GL_ARRAY_BUFFER, skyVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(skyboxVertices), skyboxVertices,GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
    startTime = SDL_GetTicks();
}

void SkyRenderer::renderSkybox(const glm::mat4& view, const glm::mat4& proj, const glm::vec3& pos) const {
    if (!skyboxShader) return;
    float currentTime = SDL_GetTicks();
    float elapsed = currentTime - startTime; //ms

    constexpr float cycleDuration = 60.f * 1000; //60s
    float t = (fmod(elapsed / cycleDuration, 1.f)); // [0..1]

    // Calculate sun/moon position based on t
    float angle = t * M_PI * 2; // full rotation over cycle

    glm::vec3 sunDir;
    glm::vec3 moonDir;

    float sunX = cos(angle);
    float sunY = sin(angle);
    sunDir = glm::normalize(glm::vec3(sunX, sunY, 0.0f));
    float moonX = -sunX;
    float moonY = -sunY;
    moonDir = glm::normalize(glm::vec3(moonX, moonY, 0.0f));

    glDepthMask(GL_FALSE);
    skyboxShader->use();

    skyboxShader->setFloat("uTime", t);
    skyboxShader->setVec3("uSunDirection", sunDir);
    skyboxShader->setVec3("uMoonDirection", moonDir);
    skyboxShader->setMat4("uView", view);
    skyboxShader->setMat4("uProjection", proj);
    glBindVertexArray(skyVAO);
    glDrawArrays(GL_TRIANGLES, 0, 36);
    glDepthMask(GL_TRUE);
}

SkyRenderer::~SkyRenderer() {
    glDeleteVertexArrays(1, &skyVAO);
    glDeleteBuffers(1, &skyVBO);
    delete skyboxShader;
}
