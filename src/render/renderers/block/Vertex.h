#ifndef VERTEX_H
#define VERTEX_H

struct Vertex {
    constexpr static int coordShift = 6;
    constexpr static int coordMask = (1 << coordShift) - 1;
    constexpr static int texShift = 1;
    constexpr static int texMask = (1 << texShift) - 1;

    //17 bits compacted: 1texX 1texY 5z 5y 5x
    unsigned int data;

    Vertex(char x, char y, char z, int textureX, int textureY) {
        data = (x & coordMask) | ((y & coordMask) << coordShift) | ((z & coordMask) << (2 * coordShift))
            | ((textureX & texMask) << (3 * coordShift)) | ((textureY & texMask) << (3 * coordShift + texShift));
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
};

#endif //VERTEX_H
