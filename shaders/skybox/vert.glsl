#version 330 core
layout (location = 0) in vec3 aPos;

out vec3 TexCoords;

uniform mat4 projection;
uniform mat4 view;
uniform vec3 pos;

void main()
{
    TexCoords = aPos;
    gl_Position = projection * view * vec4(aPos+pos, 1.0);
}