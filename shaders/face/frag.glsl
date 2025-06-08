#version 460 core

in vec3 gTexCoord;
in vec3 gFaceNormal;

out vec4 FragColor;
uniform sampler2DArray atlasTexture;
uniform vec3 lightDir;

void main() {
    // FragColor = vec4(gTexCoord.x, gTexCoord.y, 0.0, 1.0);
    // FragColor = mix(texture(atlasTexture, gTexCoord), vec4(gTexCoord.x, gTexCoord.y, 0.0, 1.0), 0.1);
    // FragColor = texture(atlasTexture, gTexCoord);

    float ambientStrength = 0.5;

    vec3 diff = vec3(max(dot(gFaceNormal, lightDir), 0.0f)+ambientStrength);
    vec4 texColor = texture(atlasTexture, gTexCoord);

    vec3 resultColor = diff * texColor.rgb;
    FragColor = vec4(resultColor, texColor.a);
}