#version 450

layout(location = 0) in vec3 fragPos;
layout(location = 1) in vec2 fragTexCoord;
layout(location = 2) in vec3 normal;
layout(location = 3) in vec3 lightPos;
layout(location = 4) in vec3 viewPos;

layout (location = 0) out vec4 outColor;

layout(binding = 1) uniform samplerCube cubeSampler;

void main() {
    vec3 I = normalize(fragPos - viewPos);
    vec3 R = reflect(I, normalize(normal));
    //outColor = vec4(texture(cubeSampler, R).rgb, 1.0);
    outColor = vec4(1, 1, 1, 1);
}
