#ifndef EFACING_H
#define EFACING_H
#include <glm/vec3.hpp>

enum Facing {
    WEST = 0,
    EAST = 1,
    SOUTH = 2,
    NORTH = 3,
    UP = 4,
    DOWN = 5,
};

constexpr glm::vec3 getDirection(Facing f) {
    switch (f) {
        case WEST:
            return {-1, 0, 0};
            break;
        case EAST:
            return {1, 0, 0};
            break;
        case SOUTH:
            return {0, 0, -1};
            break;
        case NORTH:
            break;
            return {0, 0, 1};
        case UP:
            return {0, 1, 0};
            break;
        case DOWN:
            return {0, -1, 0};
            break;
        default:
            return {0, 0, 0};
    }
}

constexpr void advanceInDirection(Facing f, glm::vec3& v) {
    switch (f) {
        case WEST:
            v.x -= 1;
            break;
        case EAST:
            v.x += 1;
            break;
        case SOUTH:
            v.z -= 1;
            break;
        case NORTH:
            v.z += 1;
            break;
        case UP:
            v.y += 1;
            break;
        case DOWN:
            v.y -= 1;
            break;
        default:
            break;
    }
}

constexpr void advanceInDirection(Facing f, glm::ivec3& v) {
    switch (f) {
        case WEST:
            v.x -= 1;
            break;
        case EAST:
            v.x += 1;
            break;
        case SOUTH:
            v.z -= 1;
            break;
        case NORTH:
            v.z += 1;
            break;
        case UP:
            v.y += 1;
            break;
        case DOWN:
            v.y -= 1;
            break;
        default:
            break;
    }
}

#endif //EFACING_H
