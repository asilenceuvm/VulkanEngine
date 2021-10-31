#version 450

layout (location = 0) in vec3 inNormal;
layout (location = 1) in vec2 inUV;
layout (location = 2) in vec3 inViewVec;
layout (location = 3) in vec3 inLightVec;
layout (location = 4) in vec3 inEyePos;
layout (location = 5) in vec3 inWorldPos;

layout (location = 0) out vec4 outFragColor;

layout(binding = 2) uniform sampler2D texSampler;

vec3 sampleTerrainLayer() {
	vec3 fragTexCoord = vec3(inUV, 1);
	vec3 color = texture(texSampler, inUV).rgb;
	return color;
}

float fog(float density)
{
	const float LOG2 = -1.442695;
	float dist = gl_FragCoord.z / gl_FragCoord.w * 0.1;
	float d = density * dist;
	return 1.0 - clamp(exp2(d * d * LOG2), 0.0, 1.0);
}

void main()
{
	vec3 N = normalize(inNormal);
	vec3 L = normalize(inLightVec);
	vec3 ambient = vec3(0.5);
	vec3 diffuse = max(dot(N, L), 0.0) * vec3(1.0);

	vec4 color = vec4((ambient + diffuse) * sampleTerrainLayer(), 1.0);

	const vec4 fogColor = vec4(0.47, 0.5, 0.67, 0.0);
	//outFragColor  = mix(color, fogColor, fog(0.25));	
	outFragColor = color;
}