#pragma once

#include <glm/glm.hpp>
#include <SDL3/SDL_events.h>

#include "Frustum.h"
#include "game/world/Chunk.h"

class Camera {
public:
    glm::vec3 Position;
    glm::vec3 Velocity;

    int viewDistance = 16;

    explicit Camera(float aspectRatio, glm::vec3 position = glm::vec3(0.0f));

    void Update(float deltaTime);

    void HandleEvent(SDL_Event& event, bool lockMouse);

    const glm::mat4& getViewMatrix() const;

    const glm::mat4& getProjectionMatrix() const;

    Frustum getFrustum() const;

    const glm::vec3& getDirection() const { return Front; }
    const glm::vec3& getPosition() const {return Position; }

    void changeAspectRatio(float aspectRatio, float fov = -1);

    const glm::vec3& getRight() const { return Right; }
    const glm::vec3& getFront() const { return Front; }
    const glm::vec3& getUp() const { return Up; }

private:
    glm::vec3 Front{1, 0, 0};
    glm::vec3 Right{0, 0, 1};
    glm::vec3 Up{0, 1, 0};

    glm::mat4 view{};
    glm::mat4 projection{};

    float Yaw = -90.0f;
    float Pitch = 0.0f;
    float Speed = 50.0f;
    float Sensitivity = 0.1f;
    float FOV = 90.0f;
    float aspectRatio{};

    const float zFar = viewDistance * Chunk::WIDTH * sqrtf(2);
    const float zNear = 0.1f;

    void updateMatrices();
};
