#pragma once

typedef uint64_t VertexData;

struct Vertex {
    constexpr static VertexData coordShift = 6;
    constexpr static VertexData coordMask = (1 << coordShift) - 1;

    constexpr static VertexData texShift = 1;
    constexpr static VertexData texMask = (1 << texShift) - 1;

    constexpr static VertexData layerShift = 11;
    constexpr static VertexData layerMask = (1 << layerShift) - 1;

    // constexpr static VertexData ;

    //31 bits compacted: 11layer 1texX 1texY 6z 6y 6x
    VertexData data;

    constexpr Vertex(char x, char y, char z, int textureX, int textureY, int layer = 0) {
        data = (x & coordMask) | ((y & coordMask) << coordShift) | ((z & coordMask) << (2 * coordShift))
            | ((textureX & texMask) << (3 * coordShift)) | ((textureY & texMask) << (3 * coordShift + texShift)) | (
                (layer & layerMask) << (3 * coordShift + 2 * texShift));
    }

    unsigned int getX() const {
        return data & coordMask;
    }

    unsigned int getY() const {
        return (data >> coordShift) & coordMask;
    }

    unsigned int getZ() const {
        return (data >> (2 * coordShift)) & coordMask;
    }

    unsigned int getTexU() const {
        return (data >> (3 * coordShift)) & texMask;
    }

    unsigned int getTexV() const {
        return (data >> (3 * coordShift + texShift)) & texMask;
    }

    unsigned int getLayer() const {
        return (data >> (3 * coordShift + 2 * texShift)) & layerMask;
    }
};
