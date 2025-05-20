#ifndef FACEINSTANCE_H
#define FACEINSTANCE_H

#include <array>

#include "Vertex.h"

struct FaceInstance {
    std::array<Vertex, 6> vertices;

    FaceInstance() = delete;

    constexpr explicit FaceInstance(const std::array<Vertex, 6> &vertices) : vertices(vertices) {
    }
};

#endif //FACEINSTANCE_H
