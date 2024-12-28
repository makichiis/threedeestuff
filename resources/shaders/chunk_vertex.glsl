#version 330 core 

layout (location = 0) in vec3 aPos;
layout (location = 1) in vec2 aTex;

uniform mat4 u_transform;
uniform mat4 u_model;

out vec2 TexCoord;

void main() {
    gl_Position = u_transform * u_model * vec4(aPos, 1.0);
    TexCoord = aTex;
}
