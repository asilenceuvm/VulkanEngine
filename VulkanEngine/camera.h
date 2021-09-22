#pragma once

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

class Camera {
public:
	void setProjection(float fov, float aspect, float near, float far);
	const glm::mat4& getProjection() const { return proj; }

	void moveCamForward(const float delta);
	void moveCamBack(const float delta);
	void moveCamLeft(const float delta);
	void moveCamRight(const float delta);
	void rotateCamera(float xoffset, float yoffset, float sensitivity);
	const glm::mat4& getView() const { return view; }

private:
    glm::mat4 proj{ 1.f };
	glm::mat4 view{ 1.f };

	glm::vec3 cameraPos = glm::vec3(0.0f, 0.0f,  3.0f);
	glm::vec3 cameraFront = glm::vec3(0.0f, 0.0f, -1.0f);
	glm::vec3 cameraUp = glm::vec3(0.0f, 1.0f,  0.0f);
    glm::vec3 direction = glm::vec3(1);
    float yaw = 0, pitch = 0;

	void updateView();
};

