#version 330 core 

layout (location = 0) in vec3 aPos;
layout (location = 1) in vec2 aTex;
layout (location = 2) in vec3 aNormal;

uniform mat4 u_transform;
uniform mat4 u_model;

out vec2 TexCoord;
out vec3 Normal;
out vec3 FragPos;

void main() {
    gl_Position = u_transform * u_model * vec4(aPos, 1.0);
    TexCoord = aTex;
    Normal = aNormal;
    FragPos = u_model[3].xyz + aPos;
}
