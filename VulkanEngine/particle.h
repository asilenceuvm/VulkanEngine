#pragma once

#include "glm/glm.hpp"

class Particle {
public:
	/* --- Physics Data --- */
	glm::vec3 velocity{ 0.f, 0.f, 0.f };
	//glm::vec3 acceleration{ 0.f, 0.f, 0.f };
	/* --- Physics Properties --- */
	float mass = 1;
	float drag = 0;
	float angular_drag = 0;
	/* --- Object Locking ---
	// 0 = unlocked, 1 = locked */
	glm::vec3 position_lock{ 0, 0, 0 };
	glm::vec3 rotation_lock{ 0, 0, 0 };
	/* --- Other Properties --- */


	Particle(float mass, glm::vec3 initialVelocity) {
		this->mass = mass;
		this->velocity = initialVelocity;
	}

	glm::vec3 force(glm::vec3 acceleration) {
		return mass * acceleration;
	}

private:

};
