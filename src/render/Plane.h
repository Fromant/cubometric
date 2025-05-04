#ifndef PLANE_H
#define PLANE_H
#include <glm/vec3.hpp>


struct Plane {
    glm::vec3 normal;
    float d; // Distance from origin
};


#endif //PLANE_H
