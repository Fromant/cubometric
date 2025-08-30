#pragma once
#include <glm/vec3.hpp>

struct AABB {
    glm::ivec3 min;
    glm::ivec3 max;

    bool intersects(const AABB& other) const {
        return (min.x <= other.max.x && max.x >= other.min.x) &&
            (min.y <= other.max.y && max.y >= other.min.y) &&
            (min.z <= other.max.z && max.z >= other.min.z);
    }
};
