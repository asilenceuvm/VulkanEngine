#version 450

layout(location = 0) in vec3 fragPos;
layout(location = 1) in vec2 fragTexCoord;
layout(location = 2) in vec3 normal;
layout(location = 3) in vec3 lightPos;
layout(location = 4) in vec3 viewPos;

layout (location = 0) out vec4 outColor;

layout(binding = 1) uniform sampler2D texSampler;

void main() {
    vec3 lightColor = vec3(1, 1, 1); //TODO: replace with passed in variable from c code

    // ambient
    float ambientStrength = 0.1;
    vec3 ambient = ambientStrength * lightColor * texture(texSampler, fragTexCoord).rgb;
  	
    // diffuse 
    vec3 norm = normalize(normal);
    vec3 lightDir = normalize(lightPos - fragPos);
    float diff = max(dot(norm, lightDir), 0.0);
    vec3 diffuse = diff * lightColor * texture(texSampler, fragTexCoord).rgb;

	// specular
    float specularStrength = 0.5;
    vec3 viewDir = normalize(viewPos - fragPos);
    vec3 reflectDir = reflect(-lightDir, norm);  
    float spec = pow(max(dot(viewDir, reflectDir), 0.0), 64);
    vec3 specular = specularStrength * spec * lightColor;  

    vec3 result = (ambient + diffuse + specular);
    //vec3 result = (ambient + diffuse);
    outColor = vec4(result, 1.0);
	//outColor = texture(texSampler, fragTexCoord);
}
