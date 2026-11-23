#include "app/camera.h"

#include <algorithm>
#include <glm/ext/matrix_clip_space.hpp>
#include <glm/ext/matrix_transform.hpp>
#include <glm/trigonometric.hpp>

namespace ih::app {

namespace {
constexpr glm::vec3 WORLD_UP{0.0f, 1.0f, 0.0f};
}

Camera::Camera (const core::CameraConfig& config, int initialViewDistance)
    : m_config (config), m_viewDistance (initialViewDistance) {
    updateView ();
    updateProjection ();
}

void Camera::update (float deltaTime) {
    const bool* keys = SDL_GetKeyboardState (nullptr);
    const float step = m_config.moveSpeed * deltaTime;

    if (keys[SDL_SCANCODE_W]) m_position += m_front * step;
    if (keys[SDL_SCANCODE_S]) m_position -= m_front * step;
    if (keys[SDL_SCANCODE_A]) m_position -= m_right * step;
    if (keys[SDL_SCANCODE_D]) m_position += m_right * step;
    if (keys[SDL_SCANCODE_SPACE]) m_position.y += step;
    if (keys[SDL_SCANCODE_LSHIFT]) m_position.y -= step;

    const bool zoom = keys[SDL_SCANCODE_C];
    if (zoom != m_zoomed) {
        m_zoomed = zoom;
        updateProjection ();
    }
    updateView ();
}

void Camera::handleEvent (const SDL_Event& event, bool mouseCaptured) {
    if (event.type == SDL_EVENT_MOUSE_MOTION && mouseCaptured) {
        m_yaw += event.motion.xrel * m_sensitivity;
        m_pitch -= event.motion.yrel * m_sensitivity;
        m_pitch = glm::clamp (m_pitch, -89.0f, 89.0f);
        updateView ();
    } else if (event.type == SDL_EVENT_KEY_DOWN) {
        if (event.key.key == SDLK_EQUALS || event.key.key == SDLK_PLUS) ++m_viewDistance;
        if (event.key.key == SDLK_MINUS) m_viewDistance = std::max (1, m_viewDistance - 1);
    }
}

void Camera::resize (uint32_t width, uint32_t height) {
    m_aspect = static_cast<float> (width) / static_cast<float> (height);
    updateProjection ();
}

void Camera::updateView () {
    glm::vec3 front;
    front.x = std::cos (glm::radians (m_yaw)) * std::cos (glm::radians (m_pitch));
    front.y = std::sin (glm::radians (m_pitch));
    front.z = std::sin (glm::radians (m_yaw)) * std::cos (glm::radians (m_pitch));
    m_front = glm::normalize (front);
    m_right = glm::normalize (glm::cross (m_front, WORLD_UP));
    m_view = glm::lookAt (m_position, m_position + m_front, WORLD_UP);
}

void Camera::updateProjection () {
    const float fov = m_zoomed ? m_config.fovDegrees / 4.0f : m_config.fovDegrees;
    // Vulkan clip space: right-handed, depth 0..1, Y flipped.
    m_projection = glm::perspectiveRH_ZO (glm::radians (fov), m_aspect, m_config.nearPlane, m_config.farPlane);
    m_projection[1][1] *= -1.0f;
}

}  // namespace ih::app
