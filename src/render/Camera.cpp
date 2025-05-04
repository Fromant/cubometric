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
    const bool *keys = SDL_GetKeyboardState(nullptr);
    float velocity = Speed * deltaTime;

    if (keys[SDL_SCANCODE_W]) Position += Front * velocity;
    if (keys[SDL_SCANCODE_S]) Position -= Front * velocity;
    if (keys[SDL_SCANCODE_A]) Position -= Right * velocity;
    if (keys[SDL_SCANCODE_D]) Position += Right * velocity;
    if (keys[SDL_SCANCODE_SPACE]) Position.y += velocity;
    if (keys[SDL_SCANCODE_LSHIFT]) Position.y -= velocity;

    updateMatrices();
}

void Camera::HandleEvent(SDL_Event &event) {
    if (event.type == SDL_EVENT_MOUSE_MOTION) {
        Yaw += event.motion.xrel * Sensitivity;
        Pitch -= event.motion.yrel * Sensitivity;
        Pitch = glm::clamp(Pitch, -89.0f, 89.0f);
        updateMatrices();
    }
}

const glm::mat4 &Camera::getViewMatrix() const {
    return view;
}

const glm::mat4 &Camera::getProjectionMatrix() const {
    return projection;
}

void Camera::changeAspectRatio(float aspectRatio) {
    this->aspectRatio = aspectRatio;
    projection = glm::perspective(glm::radians(FOV), aspectRatio, zNear, zFar);
}


void Camera::updateMatrices() {
    static const glm::vec3 Up = glm::vec3(0.0f, 1.0f, 0.0f);
    glm::vec3 front;
    front.x = cos(glm::radians(Yaw)) * cos(glm::radians(Pitch));
    front.y = sin(glm::radians(Pitch));
    front.z = sin(glm::radians(Yaw)) * cos(glm::radians(Pitch));
    Front = glm::normalize(front);
    Right = glm::normalize(glm::cross(Front, WorldUp));

    view = glm::lookAt(Position, Position + Front, Up);
}

void NormalizePlane(Plane &p) {
    // Here we calculate the magnitude of the normal to the plane (point A B C)
    // Remember that (A, B, C) is that same thing as the normal's (X, Y, Z).
    // To calculate magnitude you use the equation:  magnitude = sqrt( x^2 + y^2 + z^2)
    auto magnitude = (float) sqrt(p.normal.x * p.normal.x +
                                   p.normal.y * p.normal.y +
                                   p.normal.z * p.normal.z);

    // Then we divide the plane's values by it's magnitude.
    // This makes it easier to work with.
    p.normal /= magnitude;
    p.normal /= magnitude;
}


std::array<glm::vec4, 6> Camera::getFrustumPlanes() const {

    const glm::mat4 vp  = projection * view;

    // By default, glm uses a right handed coordinate space so we
    // will first transpose the matrix so that we can easily extract the
    // planes
    const glm::mat4 vpt = glm::transpose(vp);

    std::array<glm::vec4, 6> frustumPlanes = {
        // left, right, bottom, top
        (vpt[3] + vpt[0]),
        (vpt[3] - vpt[0]),
        (vpt[3] + vpt[1]),
        (vpt[3] - vpt[1]),
        // near, far
        (vpt[3] + vpt[2]),
        (vpt[3] - vpt[2]),
    };

    // Normalize each plane
    for (auto& plane : frustumPlanes) {
        float length = glm::length(glm::vec3(plane.x, plane.y, plane.z));
        if (length > 0.0f) {
            plane /= length;
        }
    }

    return frustumPlanes;
    //
    //
    //
    // float proj[16]; // This will hold our projection matrix
    // float modl[16]; // This will hold our modelview matrix
    // float clip[16]; // This will hold the clipping planes
    //
    // // glGetFloatv() is used to extract information about our OpenGL world.
    // // Below, we pass in GL_PROJECTION_MATRIX to abstract our projection matrix.
    // // It then stores the matrix into an array of [16].
    // glGetFloatv(GL_PROJECTION_MATRIX, proj);
    //
    // // By passing in GL_MODELVIEW_MATRIX, we can abstract our model view matrix.
    // // This also stores it in an array of [16].
    // glGetFloatv(GL_MODELVIEW_MATRIX, modl);
    //
    // // Now that we have our modelview and projection matrix, if we combine these 2 matrices,
    // // it will give us our clipping planes.  To combine 2 matrices, we multiply them.
    //
    // clip[0] = modl[0] * proj[0] + modl[1] * proj[4] + modl[2] * proj[8] + modl[3] * proj[12];
    // clip[1] = modl[0] * proj[1] + modl[1] * proj[5] + modl[2] * proj[9] + modl[3] * proj[13];
    // clip[2] = modl[0] * proj[2] + modl[1] * proj[6] + modl[2] * proj[10] + modl[3] * proj[14];
    // clip[3] = modl[0] * proj[3] + modl[1] * proj[7] + modl[2] * proj[11] + modl[3] * proj[15];
    //
    // clip[4] = modl[4] * proj[0] + modl[5] * proj[4] + modl[6] * proj[8] + modl[7] * proj[12];
    // clip[5] = modl[4] * proj[1] + modl[5] * proj[5] + modl[6] * proj[9] + modl[7] * proj[13];
    // clip[6] = modl[4] * proj[2] + modl[5] * proj[6] + modl[6] * proj[10] + modl[7] * proj[14];
    // clip[7] = modl[4] * proj[3] + modl[5] * proj[7] + modl[6] * proj[11] + modl[7] * proj[15];
    //
    // clip[8] = modl[8] * proj[0] + modl[9] * proj[4] + modl[10] * proj[8] + modl[11] * proj[12];
    // clip[9] = modl[8] * proj[1] + modl[9] * proj[5] + modl[10] * proj[9] + modl[11] * proj[13];
    // clip[10] = modl[8] * proj[2] + modl[9] * proj[6] + modl[10] * proj[10] + modl[11] * proj[14];
    // clip[11] = modl[8] * proj[3] + modl[9] * proj[7] + modl[10] * proj[11] + modl[11] * proj[15];
    //
    // clip[12] = modl[12] * proj[0] + modl[13] * proj[4] + modl[14] * proj[8] + modl[15] * proj[12];
    // clip[13] = modl[12] * proj[1] + modl[13] * proj[5] + modl[14] * proj[9] + modl[15] * proj[13];
    // clip[14] = modl[12] * proj[2] + modl[13] * proj[6] + modl[14] * proj[10] + modl[15] * proj[14];
    // clip[15] = modl[12] * proj[3] + modl[13] * proj[7] + modl[14] * proj[11] + modl[15] * proj[15];
    //
    // // Now we actually want to get the sides of the frustum.  To do this we take
    // // the clipping planes we received above and extract the sides from them.
    //
    // Frustum frustum;
    //
    // // This will extract the RIGHT side of the frustum
    // frustum.rightFace.normal.x = clip[3] - clip[0];
    // frustum.rightFace.normal.y = clip[7] - clip[4];
    // frustum.rightFace.normal.z = clip[11] - clip[8];
    // frustum.rightFace.distance = clip[15] - clip[12];
    //
    // // Now that we have a normal (A,B,C) and a distance (D) to the plane,
    // // we want to normalize that normal and distance.
    //
    // // Normalize the RIGHT side
    // NormalizePlane(frustum.rightFace);
    //
    // // This will extract the LEFT side of the frustum
    // frustum.leftFace.normal.x = clip[3] + clip[0];
    // frustum.leftFace.normal.y = clip[7] + clip[4];
    // frustum.leftFace.normal.z = clip[11] + clip[8];
    // frustum.leftFace.distance = clip[15] + clip[12];
    //
    // // Normalize the LEFT side
    // NormalizePlane(frustum.leftFace);
    //
    // // This will extract the BOTTOM side of the frustum
    // frustum.bottomFace.normal.x = clip[3] + clip[1];
    // frustum.bottomFace.normal.y = clip[7] + clip[5];
    // frustum.bottomFace.normal.z = clip[11] + clip[9];
    // frustum.bottomFace.distance = clip[15] + clip[13];
    //
    // // Normalize the BOTTOM side
    // NormalizePlane(frustum.bottomFace);
    //
    // // This will extract the TOP side of the frustum
    // frustum.topFace.normal.x = clip[3] - clip[1];
    // frustum.topFace.normal.y = clip[7] - clip[5];
    // frustum.topFace.normal.z = clip[11] - clip[9];
    // frustum.topFace.distance = clip[15] - clip[13];
    //
    // // Normalize the TOP side
    // NormalizePlane(frustum.topFace);
    //
    // // This will extract the BACK side of the frustum
    // frustum.farFace.normal.x = clip[3] - clip[2];
    // frustum.farFace.normal.y = clip[7] - clip[6];
    // frustum.farFace.normal.z = clip[11] - clip[10];
    // frustum.farFace.distance = clip[15] - clip[14];
    //
    // // Normalize the BACK side
    // NormalizePlane(frustum.farFace);
    //
    // // This will extract the FRONT side of the frustum
    // frustum.nearFace.normal.x = clip[3] + clip[2];
    // frustum.nearFace.normal.y = clip[7] + clip[6];
    // frustum.nearFace.normal.z = clip[11] + clip[10];
    // frustum.nearFace.distance = clip[15] + clip[14];
    //
    // // Normalize the FRONT side
    // NormalizePlane(frustum.nearFace);
    //
    // return frustum;
}
