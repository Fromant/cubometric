#version 450

layout (location = 0) in vec3 vNormal;
layout (location = 1) in vec2 vUv;
layout (location = 2) in flat uint vLayer;
layout (location = 3) in float vLight;

layout (set = 0, binding = 0) uniform sampler2DArray uAtlas;

layout (push_constant) uniform Push {
    mat4 viewProj;
    vec3 sunDir;
    float ambient;
    vec3 chunkOrigin;
    float pad;
} push;

layout (location = 0) out vec4 oColor;

void main () {
    const vec4 tex = texture (uAtlas, vec3 (vUv, float (vLayer)));
    const float diff = max (dot (normalize (vNormal), push.sunDir), 0.0) + push.ambient;
    oColor = vec4 (tex.rgb, 1.0);  // DEBUG: lighting off
}
