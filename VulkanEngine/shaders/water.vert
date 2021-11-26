#version 450

layout(location = 0) in vec3 position;
layout(location = 1) in vec3 color;
layout(location = 2) in vec3 inNormal;
layout(location = 3) in vec2 inTexCoord;


layout(location = 0) out vec3 fragPos;
layout(location = 1) out vec2 fragTexCoord;
layout(location = 2) out vec3 normal;
layout(location = 3) out vec3 lightPos;
layout(location = 4) out vec3 viewPos;


layout(binding = 0) uniform UniformBufferObject {
	mat4 model;
	mat4 view;
	mat4 proj;
	vec3 lightPos;
	vec3 viewPos;
	float time;
} ubo;


vec3 GerstnerWave (vec4 wave, vec3 p, inout vec3 tangent, inout vec3 binormal) {
	float steepness = wave.z;
	float wavelength = wave.w;
	float k = 2 * 3.14 / wavelength;
	float c = sqrt(9.8 / k);
	vec2 d = normalize(wave.xy);
	float f = k * (dot(d, p.xz) - c * ubo.time);
	float a = steepness / k;
	
	//p.x += d.x * (a * cos(f));
	//p.y = a * sin(f);
	//p.z += d.y * (a * cos(f));

	tangent += vec3(
		-d.x * d.x * (steepness * sin(f)),
		d.x * (steepness * cos(f)),
		-d.x * d.y * (steepness * sin(f))
	);
	binormal += vec3(
		-d.x * d.y * (steepness * sin(f)),
		d.y * (steepness * cos(f)),
		-d.y * d.y * (steepness * sin(f))
	);
	return vec3(
		d.x * (a * cos(f)),
		a * sin(f),
		d.y * (a * cos(f))
	);
}

void main() {
	fragTexCoord = inTexCoord;

	vec4 waveA = vec4(1,  1,  0.3, 4);
	vec4 waveB = vec4(0,  1,  0.1, 4);
	vec4 waveC = vec4(1,  0,  0.2, 5);
	vec4 waveD = vec4(-1,  -1,  0.1, 2);
	
	vec3 tangent = vec3(1, 0, 0);
	vec3 binormal = vec3(0, 0, 1);
	vec3 p = position;
	p += GerstnerWave(waveA, position, tangent, binormal);
	p += GerstnerWave(waveB, position, tangent, binormal);
	p += GerstnerWave(waveC, position, tangent, binormal);
	p += GerstnerWave(waveD, position, tangent, binormal);
	vec3 pos = p;
	fragPos = vec3(ubo.model * vec4(pos, 1.0));
    gl_Position = ubo.proj * ubo.view * vec4(fragPos, 1.0);

	vec3 norm = normalize(cross(binormal, tangent));
	normal = mat3(transpose(inverse(ubo.model))) * norm;  
	
	lightPos = ubo.lightPos;
	viewPos = ubo.viewPos;
}
