#include "Camera.h"

#include <SDL3/SDL.h>
#include <glm/ext/matrix_transform.hpp> // glm::translate, glm::rotate, glm::scale, glm::lookAt
#include <glm/ext/matrix_clip_space.hpp> // glm::perspective

constexpr glm::vec3 WorldUp{0, 1, 0};

Camera::Camera(float aspectRatio, glm::vec3 position) : Position(position), Velocity(0.0f, 0.0f, 0.0f) {
    updateMatrices();
    changeAspectRatio(aspectRatio);
}

void Camera::Update(float deltaTime) {
    const bool* keys = SDL_GetKeyboardState(nullptr);
    float velocity = Speed * deltaTime;

    if (keys[SDL_SCANCODE_W]) Position += Front * velocity;
    if (keys[SDL_SCANCODE_S]) Position -= Front * velocity;
    if (keys[SDL_SCANCODE_A]) Position -= Right * velocity;
    if (keys[SDL_SCANCODE_D]) Position += Right * velocity;
    if (keys[SDL_SCANCODE_SPACE]) Position.y += velocity;
    if (keys[SDL_SCANCODE_LSHIFT]) Position.y -= velocity;
    if (keys[SDL_SCANCODE_C]) changeAspectRatio(aspectRatio, FOV / 4);
    else changeAspectRatio(aspectRatio);

    updateMatrices();
}

void Camera::HandleEvent(SDL_Event& event, bool lockMouse) {
    if (event.type == SDL_EVENT_MOUSE_MOTION && lockMouse) {
        Yaw += event.motion.xrel * Sensitivity;
        Pitch -= event.motion.yrel * Sensitivity;
        Pitch = glm::clamp(Pitch, -89.0f, 89.0f);
        updateMatrices();
    }
    else if (event.type == SDL_EVENT_KEY_DOWN) {
        if (event.key.key == SDLK_EQUALS) viewDistance++;
        else if (event.key.key == SDLK_MINUS) viewDistance--;
    }
}

const glm::mat4& Camera::getViewMatrix() const {
    return view;
}

const glm::mat4& Camera::getProjectionMatrix() const {
    return projection;
}

void Camera::changeAspectRatio(float aspectRatio, float fov) {
    this->aspectRatio = aspectRatio;
    if (fov < 0) fov = FOV;
    projection = glm::perspective(glm::radians(fov), aspectRatio, zNear, zFar);
}


void Camera::updateMatrices() {
    glm::vec3 front;
    front.x = cos(glm::radians(Yaw)) * cos(glm::radians(Pitch));
    front.y = sin(glm::radians(Pitch));
    front.z = sin(glm::radians(Yaw)) * cos(glm::radians(Pitch));
    Front = glm::normalize(front);
    Right = glm::normalize(glm::cross(Front, WorldUp));
    Up = glm::cross(Right, Front);
    view = glm::lookAt(Position, Position + Front, WorldUp);
}


Frustum Camera::getFrustum() const {
    //black magic code i don't understand it
    const glm::mat4 vp = projection * view;

    const glm::mat4 vpt = glm::transpose(vp);

    std::array<Plane, 6> frustumPlanes = {
            // left, right, bottom, top
            Plane(vpt[3] + vpt[0]),
            Plane(vpt[3] - vpt[0]),
            Plane(vpt[3] + vpt[1]),
            Plane(vpt[3] - vpt[1]),
            // near, far
            Plane(vpt[3] + vpt[2]),
            Plane(vpt[3] - vpt[2]),
        };

    // Normalize each plane
    for (auto& plane : frustumPlanes) {
        float length = sqrtf(plane.data.x * plane.data.x + plane.data.y * plane.data.y + plane.data.z * plane.data.z);
        if (length > 0.0f) {
            plane.data /= length;
        }
    }

    return frustumPlanes;
}
