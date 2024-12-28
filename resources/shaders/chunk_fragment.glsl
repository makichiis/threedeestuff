#version 330 core 

out vec4 FragColor;
in vec2 TexCoord;

uniform sampler2D map;

void main() {
    FragColor = texture(map, TexCoord);
}