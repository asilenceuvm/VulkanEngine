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


struct waveInfo {
	float w;
	float steepness;
	float amplitude;
	float phase_const;
	vec2 dir;
};

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
	waveInfo waves[3];
	waves[0].w = 1;
	waves[0].steepness = 1;
	waves[0].amplitude = 1;
	waves[0].phase_const = 1;
	waves[0].dir = vec2(1, 0);
	waves[1].w = 1.1;
	waves[1].steepness = 1.1;
	waves[1].amplitude = 1.3;
	waves[1].phase_const = 1.5;
	waves[1].dir = vec2(0, 1);
	waves[2].w = 0.5;
	waves[2].steepness = 0.3;
	waves[2].amplitude = 0.7;
	waves[2].phase_const = 0.9;
	waves[2].dir = vec2(1 / sqrt(2), 1/sqrt(2));

	//float y = position.y +  amplitude * sin(wavelength * (ubo.time + position.x)); 
	//float yPos = 0;
	//for(int i = 0; i < waves.length(); i++) {
		//yPos += (waves[i].steepness * waves[i].amplitude) * waves[i].dir.y * cos(waves[i].w * (dot(waves[i].dir, position.xz) + waves[i].phase_const * ubo.time));
	//}
	//vec3 pos = vec3(position.x, yPos, position.z);
	//fragPos = vec3(ubo.model * vec4(pos, 1.0));
	//vec3 B = normalize(vec3(1.0, -amplitude * 1 * cos(ubo.time + position.x),0.0));
	//vec3 T = normalize(vec3(-amplitude * 1 * cos(ubo.time + position.x),0.0,1.0));
	//vec3 norm = cross(B, T);
	//vec3 Wi = dot(2 * amplitude,  pow(sin((dot(dir, position.xz) * wavelength + ubo.time * (speed * (2 / wavelength))) + 1) / 2, sharpness));
    //gl_Position = ubo.proj * ubo.view * vec4(fragPos, 1.0);

	fragTexCoord = inTexCoord;
	//vec3 normal = normalize(cross(binormal, tangent));

	//float xNorm = 0, yNorm = 0, zNorm = 0;
	//for(int i = 0; i < waves.length(); i++) {
	//	float WA = waves[i].w * waves[i].amplitude;
	//	float S0 = sin(waves[i].w * waves[i].dir + waves[i].phase_const * ubo.time);
	//	xNorm -= waves[i].dir.x 
	//}

	//vec3 norm = inNormal;
	//normal = mat3(transpose(inverse(ubo.model))) * norm;  
	vec4 waveA = vec4(1,1,0.3,13);
	vec4 waveB = vec4(0,1,0.2,15);
	vec4 waveC = vec4(1,0,0.1,18);
	vec4 waveD = vec4(0.3,0.7,0.1,5);
	vec4 waveE = vec4(1,1,0.2,5);
	
	vec3 tangent = vec3(1, 0, 0);
	vec3 binormal = vec3(0, 0, 1);
	vec3 p = position;
	p += GerstnerWave(waveA, position, tangent, binormal);
	p += GerstnerWave(waveB, position, tangent, binormal);
	p += GerstnerWave(waveC, position, tangent, binormal);
	vec3 pos = p;
	fragPos = vec3(ubo.model * vec4(pos, 1.0));
    gl_Position = ubo.proj * ubo.view * vec4(fragPos, 1.0);

	vec3 norm = normalize(cross(binormal, tangent));
	normal = mat3(transpose(inverse(ubo.model))) * norm;  
	
	lightPos = ubo.lightPos;
	viewPos = ubo.viewPos;
}
