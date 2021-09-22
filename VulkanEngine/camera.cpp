#include "camera.h"

#include <iostream>

#include "spdlog/spdlog.h"
#include "glm/gtx/string_cast.hpp"

void Camera::setProjection(float fov, float aspect, float nearVal, float farVal) {
	proj = glm::perspective(fov, aspect, nearVal, farVal);
	proj[1][1] *= -1;
}

void Camera::moveCamForward(const float delta) {
	cameraPos += delta * cameraFront;
	updateView();
}
void Camera::moveCamBack(const float delta) {
	cameraPos -= delta * cameraFront;
	updateView();
}
void Camera::moveCamLeft(const float delta) {
	cameraPos -= glm::normalize(glm::cross(cameraFront, cameraUp)) * delta;
	updateView();
}
void Camera::moveCamRight(const float delta) {
	cameraPos += glm::normalize(glm::cross(cameraFront, cameraUp)) * delta;
	updateView();
}

void Camera::rotateCamera(float xoffset, float yoffset, float sensitivity) {
	yaw += xoffset * sensitivity;
	pitch += yoffset * sensitivity;

	//clamp y look
	if (pitch > 89.0f) {
		pitch = 89.0f;
	}
	if (pitch < -89.0f) {
		pitch = -89.0f;
	}
	
	direction.x = cos(glm::radians(yaw)) * cos(glm::radians(pitch));
	direction.y = sin(glm::radians(pitch));
	direction.z = sin(glm::radians(yaw)) * cos(glm::radians(pitch));
	cameraFront = glm::normalize(direction);

	updateView();
}

void Camera::updateView() {
	view = glm::lookAt(cameraPos, cameraPos + cameraFront, cameraUp);
}
