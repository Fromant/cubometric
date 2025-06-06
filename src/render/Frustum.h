#pragma once

#include <array>

#include <glm/vec3.hpp>
#include <glm/geometric.hpp>

#include "utils/AABB.hpp"
#include "Plane.h"


struct Frustum {
    std::array<Plane, 6> planes;

    Frustum(Plane top, Plane bottom, Plane right, Plane left, Plane far, Plane near) : planes{
            top, bottom, right, left, far, near
        } {}

    Frustum(const std::array<Plane, 6>& planes) : planes(planes) {}

    bool isAABBVisible(const AABB& box) const {
        const glm::vec3& vmin = box.min;
        const glm::vec3& vmax = box.max;

        for (const auto& g : planes) {
            if ((glm::dot(g.data, glm::vec4(vmin.x, vmin.y, vmin.z, 1.0f)) < 0.0) &&
                (glm::dot(g.data, glm::vec4(vmax.x, vmin.y, vmin.z, 1.0f)) < 0.0) &&
                (glm::dot(g.data, glm::vec4(vmin.x, vmax.y, vmin.z, 1.0f)) < 0.0) &&
                (glm::dot(g.data, glm::vec4(vmax.x, vmax.y, vmin.z, 1.0f)) < 0.0) &&
                (glm::dot(g.data, glm::vec4(vmin.x, vmin.y, vmax.z, 1.0f)) < 0.0) &&
                (glm::dot(g.data, glm::vec4(vmax.x, vmin.y, vmax.z, 1.0f)) < 0.0) &&
                (glm::dot(g.data, glm::vec4(vmin.x, vmax.y, vmax.z, 1.0f)) < 0.0) &&
                (glm::dot(g.data, glm::vec4(vmax.x, vmax.y, vmax.z, 1.0f)) < 0.0)) {
                // Not visible - all returned negative
                return false;
            }
        }

        // Potentially visible
        return true;
    }
};
