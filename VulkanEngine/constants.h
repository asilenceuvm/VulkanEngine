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
		alignas(16) float displacementFactor = 32.0f;
		alignas(16) float tessellationFactor = 0.75f;
		alignas(16) glm::vec2 viewportDim;
		alignas(16) float tessellatedEdgeSize = 20.0f;
	};

	class Frustum
	{
	public:
		enum side { LEFT = 0, RIGHT = 1, TOP = 2, BOTTOM = 3, BACK = 4, FRONT = 5 };
		std::array<glm::vec4, 6> planes;

		void update(glm::mat4 matrix)
		{
			planes[LEFT].x = matrix[0].w + matrix[0].x;
			planes[LEFT].y = matrix[1].w + matrix[1].x;
			planes[LEFT].z = matrix[2].w + matrix[2].x;
			planes[LEFT].w = matrix[3].w + matrix[3].x;

			planes[RIGHT].x = matrix[0].w - matrix[0].x;
			planes[RIGHT].y = matrix[1].w - matrix[1].x;
			planes[RIGHT].z = matrix[2].w - matrix[2].x;
			planes[RIGHT].w = matrix[3].w - matrix[3].x;

			planes[TOP].x = matrix[0].w - matrix[0].y;
			planes[TOP].y = matrix[1].w - matrix[1].y;
			planes[TOP].z = matrix[2].w - matrix[2].y;
			planes[TOP].w = matrix[3].w - matrix[3].y;

			planes[BOTTOM].x = matrix[0].w + matrix[0].y;
			planes[BOTTOM].y = matrix[1].w + matrix[1].y;
			planes[BOTTOM].z = matrix[2].w + matrix[2].y;
			planes[BOTTOM].w = matrix[3].w + matrix[3].y;

			planes[BACK].x = matrix[0].w + matrix[0].z;
			planes[BACK].y = matrix[1].w + matrix[1].z;
			planes[BACK].z = matrix[2].w + matrix[2].z;
			planes[BACK].w = matrix[3].w + matrix[3].z;

			planes[FRONT].x = matrix[0].w - matrix[0].z;
			planes[FRONT].y = matrix[1].w - matrix[1].z;
			planes[FRONT].z = matrix[2].w - matrix[2].z;
			planes[FRONT].w = matrix[3].w - matrix[3].z;

			for (auto i = 0; i < planes.size(); i++)
			{
				float length = sqrtf(planes[i].x * planes[i].x + planes[i].y * planes[i].y + planes[i].z * planes[i].z);
				planes[i] /= length;
			}
		}
		
		bool checkSphere(glm::vec3 pos, float radius)
		{
			for (auto i = 0; i < planes.size(); i++)
			{
				if ((planes[i].x * pos.x) + (planes[i].y * pos.y) + (planes[i].z * pos.z) + planes[i].w <= -radius)
				{
					return false;
				}
			}
			return true;
		}
	};
}
