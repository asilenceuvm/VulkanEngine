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
		float time;
	};

	struct CubeMapUBO {
		alignas(16) glm::mat4 view;
		alignas(16) glm::mat4 proj;
	}; 

	struct TesselationUBO {
		alignas(16) glm::mat4 projection;
		alignas(16) glm::mat4 modelview;
		alignas(16) glm::vec4 lightPos;
		alignas(16) glm::vec4 frustumPlanes[6];
		alignas(16) float displacementFactor;
		alignas(16) float tessellationFactor;
		alignas(16) glm::vec2 viewportDim;
		alignas(16) float tessellatedEdgeSize;
	};
}
