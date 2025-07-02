#version 330 core
in vec2 vTexCoord;
out vec4 FragColor;

uniform sampler2D uAtlas;
uniform float uOpacity;

void main() {
    vec4 texColor = texture(uAtlas, vTexCoord);

    // Discard transparent pixels
    if(texColor.a < 0.1) discard;

    texColor.a *= uOpacity;

    FragColor = texColor;
}