#version 440 core
layout(location = 0) in int aPos;

uniform mat4 view;
uniform mat4 projection;
uniform ivec3 chunkCoords; // Chunk position in world (e.g., chunkCoords * 16 gives world offset)
const int CHUNK_SIZE = 32; // Adjust if your chunk size differs

out vec3 TexCoord;

void main() {
    // Extract local position within the chunk (6 bits each)
    int x = (aPos & 0x3F);
    int y = (aPos >> 6) & 0x3F;
    int z = (aPos >> 12) & 0x3F;

    // Calculate world position by adding chunk offset
    vec3 worldPos = vec3(x, y, z) + vec3(chunkCoords * CHUNK_SIZE);

    // Texture coordinates (from bits 12 and 13)
    TexCoord = vec3((aPos >> 18) & 1, (aPos >> 19) & 1, (aPos>>20)&0x7FF);

    // Transform to clip space
    gl_Position = projection * view * vec4(worldPos, 1.0);
}