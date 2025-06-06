#pragma once

#include <glm/vec3.hpp>
#include <glm/vec4.hpp>


struct Plane {
    glm::vec4 data;

    Plane() = default;
    Plane(const glm::vec3& n, float d) : data(n, d) {}
    explicit Plane(const glm::vec4& v): data(v) {}
};
