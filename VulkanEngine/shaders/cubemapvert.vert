#version 450

layout(location = 0) in vec3 position;
layout(location = 0) out vec3 fragTexCoord;

layout(binding = 0) uniform UniformBufferObject {
	mat4 view;
	mat4 proj;
} ubo;

void main() {
	fragTexCoord = position;
    gl_Position = ubo.proj * ubo.view * vec4(position, 1.0);
}