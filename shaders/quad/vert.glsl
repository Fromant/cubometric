#version 330 core
layout(location = 0) in vec3 aPos; // Quad vertices (-1 to 1)
layout(location = 1) in vec3 aInstanceData; // xyz=world pos
layout(location = 2) in float scale;
layout(location = 3) in float size;
layout(location = 4) in vec2 uv; // uv start

uniform mat4 uMVP;
uniform mat4 uSkyRotation;  // Rotation matrix for day/night cycle
uniform vec3 uCameraRight;
uniform vec3 uCameraUp;
uniform vec3 uCameraPos;

out vec2 vTexCoord;

void main() {

    // Rotate the star position around world origin
    vec3 rotatedPos = (uSkyRotation * vec4(aInstanceData, 1.0)).xyz;

    // Place star at rotated position
    vec3 worldCenter = rotatedPos;

    // Billboard expansion
    vec3 posWorld = worldCenter
            + uCameraRight * aPos.x * scale
            + uCameraUp * aPos.y * scale;

    vTexCoord = uv + (aPos.xy * 0.5 + 0.5) * size;
    gl_Position = uMVP * vec4(posWorld, 1.0);
}