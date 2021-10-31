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
    
    // specular
    vec3 lightDir = normalize(lightPos - fragPos);
    vec3 norm = normalize(normal);
    float specularStrength = 0.5;
    vec3 viewDir = normalize(viewPos - fragPos);
    vec3 reflectDir = reflect(-lightDir, norm);  
    float spec = pow(max(dot(viewDir, reflectDir), 0.0), 64);
    vec3 specular = specularStrength * spec * vec3(1,1,1);  

    vec3 watercolor = vec3(0.25, 0.5, 1);
    float reflectStrength = 0.2;
    vec3 result = reflectStrength * texture(cubeSampler, R).rgb + watercolor + specular;

    outColor = vec4(result, 0.5);
}
