#version 460 core

layout(triangles) in;
layout(triangle_strip, max_vertices = 3) out;

layout(location = 1) in vec3 vWorldPos[];
layout(location = 0) in vec3 TexCoord[];

layout(location = 0) out vec3 gFaceNormal;
layout(location = 1) out vec3 gTexCoord;

void main() {
    vec3 edge1 = vWorldPos[1] - vWorldPos[0];
    vec3 edge2 = vWorldPos[2] - vWorldPos[0];

    gFaceNormal = normalize(cross(edge1, edge2));

    for (int i=0; i<3; i++) {
        gl_Position = gl_in[i].gl_Position;
        gTexCoord = TexCoord[i];
        EmitVertex();
    }
    EndPrimitive();
}