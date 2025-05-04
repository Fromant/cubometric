#version 440 core
in vec2 TexCoord;
out vec4 FragColor;

uniform sampler2D atlasTexture;

void main() {
    FragColor = vec4(TexCoord.x, TexCoord.y, 0.0, 1.0);
    FragColor = mix(texture(atlasTexture, TexCoord), vec4(TexCoord.x, TexCoord.y, 0.0, 1.0), 0.1);
}