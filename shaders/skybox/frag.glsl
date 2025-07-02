#version 330 core
in vec3 vTexCoords;
out vec4 FragColor;

uniform float uTime;
uniform vec3 uSunDirection;

// Sky color function (optimized)
vec3 getSkyColor(vec3 dir) {
    float t = dot(dir, uSunDirection);
    float sunsetOffset = 0.55;

    vec3 nightColor = vec3(0.01, 0.03, 0.06);  // Darker for better stars
    vec3 dayColor = vec3(0.2, 0.5, 1.0);
    vec3 sunsetColor = vec3(1.0, 0.5, 0.2);

    float sunsetFactor = smoothstep(0, 1, max(1.0 - abs(uSunDirection.y) / sunsetOffset, 0.0));
    vec3 skyColor = mix(nightColor, dayColor, clamp(uSunDirection.y / sunsetOffset + 1.0, 0.0, 1.0));
    return mix(skyColor, sunsetColor, sunsetFactor);
}

void main() {
    vec3 dir = normalize(vTexCoords);
    vec3 color = getSkyColor(dir);
    FragColor = vec4(color, 1.0);
}