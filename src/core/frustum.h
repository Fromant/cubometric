#pragma once

#include <array>
#include <cmath>

#include <glm/glm.hpp>

namespace ih::core {

struct Plane {
    glm::vec4 data{0.0f};  // xyz = normal, w = distance

    Plane () = default;
    explicit Plane (const glm::vec4& d) : data (d) { normalize (); }

    void normalize () {
        const float length = std::sqrt (data.x * data.x + data.y * data.y + data.z * data.z);
        if (length > 0.0f) data /= length;
    }

    float distance (const glm::vec3& point) const { return glm::dot (glm::vec3 (data), point) + data.w; }
};

struct AABB {
    glm::vec3 min{0.0f};
    glm::vec3 max{0.0f};
};

using Frustum = std::array<Plane, 6>;

// Gribb-Hartmann plane extraction from a view-projection matrix.
inline Frustum extractFrustum (const glm::mat4& vp) {
    const glm::mat4 t = glm::transpose (vp);
    Frustum frustum = {
        Plane (t[3] + t[0]),  // left
        Plane (t[3] - t[0]),  // right
        Plane (t[3] + t[1]),  // bottom
        Plane (t[3] - t[1]),  // top
        Plane (t[3] + t[2]),  // near
        Plane (t[3] - t[2]),  // far
    };
    return frustum;
}

inline bool intersects (const Frustum& frustum, const AABB& box) {
    for (const auto& plane : frustum) {
        // Positive vertex: the corner furthest along the plane normal.
        const glm::vec3 positive (plane.data.x >= 0 ? box.max.x : box.min.x,
                                  plane.data.y >= 0 ? box.max.y : box.min.y,
                                  plane.data.z >= 0 ? box.max.z : box.min.z);
        if (plane.distance (positive) < 0.0f) return false;
    }
    return true;
}

}  // namespace ih::core
