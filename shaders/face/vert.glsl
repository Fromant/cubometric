#version 440 core
layout(location = 0) in int aPos;

uniform mat4 view;
uniform mat4 projection;
uniform ivec3 chunkCoords; // Chunk position in world (e.g., chunkCoords * 16 gives world offset)
const int CHUNK_SIZE = 16; // Adjust if your chunk size differs

out vec2 TexCoord;

void main() {
    // Extract local position within the chunk (4 bits each)
    int x = (aPos & 0x1F);
    int y = (aPos >> 5) & 0x1F;
    int z = (aPos >> 10) & 0x1F;

    // Calculate world position by adding chunk offset
    vec3 worldPos = vec3(x, y, z) + vec3(chunkCoords * CHUNK_SIZE);

    // Texture coordinates (from bits 12 and 13)
    TexCoord = vec2((aPos >> 15) & 1, (aPos >> 16) & 1);

    // Transform to clip space
    gl_Position = projection * view * vec4(worldPos, 1.0);
}