#version 450

layout(location = 0) in vec3 position;
layout(location = 1) in vec3 color;
layout(location = 2) in vec3 inNormal;
layout(location = 3) in vec2 inTexCoord;


layout(location = 0) out vec3 fragPos;
layout(location = 1) out vec2 fragTexCoord;
layout(location = 2) out vec3 normal;
layout(location = 3) out vec3 lightPos;


layout(binding = 0) uniform UniformBufferObject {
	mat4 model;
	mat4 view;
	mat4 proj;
	vec3 lightPos;
} ubo;

void main() {
	fragPos = vec3(ubo.model * vec4(position, 1.0));
    gl_Position = ubo.proj * ubo.view * vec4(fragPos, 1.0);

	fragTexCoord = inTexCoord;
	normal = mat3(transpose(inverse(ubo.model))) * inNormal;  
	lightPos = ubo.lightPos;
}