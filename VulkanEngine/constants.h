#pragma once

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include "glm/glm.hpp"
#include "glm/gtc/constants.hpp"
#include "glm/gtx/string_cast.hpp"


namespace Constants {
	struct ObjectUBO {
		alignas(16) glm::mat4 model;
		alignas(16) glm::mat4 view;
		alignas(16) glm::mat4 proj;
		alignas(16) glm::vec3 lightPos;
		alignas(16) glm::vec3 viewPos;
	};

	struct CubeMapUBO {
		alignas(16) glm::mat4 view;
		alignas(16) glm::mat4 proj;
	}; 
}
