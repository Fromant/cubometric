#ifndef EFACING_H
#define EFACING_H
#include <glm/vec3.hpp>

enum Facing {
    WEST = 0,
    EAST = 1,
    NORTH = 2,
    SOUTH = 3,
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
        case NORTH:
            return {0, 0, -1};
        case SOUTH:
            return {0, 0, 1};
        case UP:
            return {0, 1, 1};
        case DOWN:
            return {0, -1, 0};
    }
}

#endif //EFACING_H
