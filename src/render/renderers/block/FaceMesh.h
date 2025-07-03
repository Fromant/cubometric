#ifndef FACEINSTANCE_H
#define FACEINSTANCE_H

#include <array>

#include "Vertex.h"

struct FaceMesh {
    std::array<Vertex, 6> vertices;

    FaceMesh() = delete;

    constexpr explicit FaceMesh(const std::array<Vertex, 6> &vertices) : vertices(vertices) {
    }
};

#endif //FACEINSTANCE_H
