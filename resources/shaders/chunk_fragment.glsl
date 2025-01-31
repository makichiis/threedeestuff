#version 330 core 

out vec4 FragColor;

in vec3 FragPos;
in vec2 TexCoord;
in vec3 Normal;

uniform vec3 u_lightpos;
uniform vec3 u_camerapos;

uniform sampler2D map;

void main() {
    vec3 normal = normalize(Normal);
    FragColor = texture(map, TexCoord) - 0.05 - (0.075 * (1 - normal.y)) + (0.05 * (1 - abs(normal.z))) - (0.05 * -normal.y);
    //FragColor = vec4(0.85, 0.85, 0.85, 1.0) - 0.05 - (0.075 * (1 - normal.y)) + (0.05 * (1 - abs(normal.z))) - (0.05 * -normal.y);

    vec3 fogColor = vec3(0.52, 0.71, 0.83);
    float fogDensity = 0.5;
    float fogStart = 1000.0;
    float fogEnd = 100.0;

    float d = length(u_camerapos - FragPos);
    float fogFactor = smoothstep(fogStart, fogEnd, d);
    vec3 finalColor = mix(fogColor, FragColor.rgb, fogFactor);

    //FragColor = vec4(finalColor, 1.0);
}