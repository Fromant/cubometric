#ifndef VERTEX_H
#define VERTEX_H

struct Vertex {
    //14 bits compacted: 1texX 1texY 5z 5y 5x
    unsigned int data;

    Vertex(char x, char y, char z, int textureX, int textureY) {
        data = (x & 0x1F) + ((y & 0x1F) << 5) + ((z & 0x1F) << 10) + ((textureX & 1) << 15) + ((textureY & 1) << 16);
    }
};

#endif //VERTEX_H
