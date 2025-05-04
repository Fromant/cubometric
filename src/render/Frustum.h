#ifndef FRUSTUM_H
#define FRUSTUM_H

#include "Plane.h"

struct Frustum {
    Plane topFace;
    Plane bottomFace;

    Plane rightFace;
    Plane leftFace;

    Plane farFace;
    Plane nearFace;
};

#endif //FRUSTUM_H
