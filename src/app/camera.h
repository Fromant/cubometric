#pragma once

#include <SDL3/SDL.h>
#include <glm/glm.hpp>

#include "core/config.h"
#include "core/frustum.h"

namespace ih::app {

// Fly camera, owned by the event thread. Its state is published to the
// render thread through FrameParams; matrices are rebuilt here.
class Camera {
public:
    explicit Camera (const core::CameraConfig& config, int initialViewDistance);

    void update (float deltaTime);
    void handleEvent (const SDL_Event& event, bool mouseCaptured);
    void resize (uint32_t width, uint32_t height);

    glm::mat4 viewProjection () const { return m_projection * m_view; }
    glm::vec3 position () const { return m_position; }
    int viewDistance () const { return m_viewDistance; }
    core::Frustum frustum () const { return core::extractFrustum (viewProjection ()); }

private:
    void updateView ();
    void updateProjection ();

    core::CameraConfig m_config{};
    glm::vec3 m_position{0.0f, 80.0f, 0.0f};
    glm::vec3 m_front{0.0f, 0.0f, -1.0f};
    glm::vec3 m_right{1.0f, 0.0f, 0.0f};
    float m_yaw = -90.0f;
    float m_pitch = -10.0f;
    float m_sensitivity = 0.1f;
    float m_aspect = 16.0f / 9.0f;
    bool m_zoomed = false;
    int m_viewDistance = 8;

    glm::mat4 m_view{1.0f};
    glm::mat4 m_projection{1.0f};
};

}  // namespace ih::app
