#version 450

// Packed vertex: two uint32 words (see world/mesher.h).
layout (location = 0) in uvec2 aData;

layout (push_constant) uniform Push {
    mat4 viewProj;
    vec3 sunDir;
    float ambient;
    vec3 chunkOrigin;
    float pad;
} push;

layout (location = 0) out vec3 vNormal;
layout (location = 1) out vec2 vUv;
layout (location = 2) out flat uint vLayer;
layout (location = 3) out float vLight;

const vec3 NORMALS[6] = vec3[6] (
    vec3 (1, 0, 0), vec3 (-1, 0, 0),
    vec3 (0, 1, 0), vec3 (0, -1, 0),
    vec3 (0, 0, 1), vec3 (0, 0, -1)
);

void main () {
    const uint lo = aData.x;
    const uint hi = aData.y;
    const uint x = lo & 0x3Fu;
    const uint y = (lo >> 6) & 0x1FFu;
    const uint z = (lo >> 15) & 0x3Fu;
    const uint face = (lo >> 21) & 0x7u;
    const uint light = (lo >> 24) & 0xFFu;
    const uint layer = hi & 0x7FFu;

    const vec3 local = vec3 (float (x), float (y), float (z));
    gl_Position = push.viewProj * vec4 (local + push.chunkOrigin, 1.0);

    vNormal = NORMALS[face];
    // UVs in tile units; repeat addressing tiles greedy-merged quads.
    // Side faces negate v so texture "up" matches world up.
    if (face == 2u || face == 3u) {
        vUv = vec2 (local.x, local.z);
    } else if (face == 0u || face == 1u) {
        vUv = vec2 (local.z, -local.y);
    } else {
        vUv = vec2 (local.x, -local.y);
    }
    vLayer = layer;
    vLight = float (light) / 255.0;
}
