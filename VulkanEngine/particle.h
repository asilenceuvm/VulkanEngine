#pragma once

#include "glm/glm.hpp"
#include "glm/gtx/string_cast.hpp"

typedef struct {
	float width;
	float height;
	float depth;
	float mass;
	float momentOfInertia;
} BoxShape;

class Particle {
public:
	/* --- Physics Data --- */
	glm::vec3 linearVelocity{ 0.f, 0.f, 0.f };
	glm::vec3 angularVelocity{ 0.f, 0.f, 0.f };
	/* --- Physics Properties --- */
	float drag = 0.001f;
	float angular_drag = 0;
	BoxShape shape;
	/* --- Object Locking ---
	// 0 = unlocked, 1 = locked */
	glm::vec3 position_lock{ 0, 0, 0 };
	glm::vec3 rotation_lock{ 0, 0, 0 };
	/* --- Other Properties --- */
	glm::vec3 force{ 0.f, 0.f ,0.f };
	glm::vec3 torque{ 0.f, 0.f ,0.f };
	

	Particle(float mass, glm::vec3 initialVelocity, glm::vec3 initialAngular) {
		this->linearVelocity = initialVelocity;
		this->angularVelocity = initialAngular;
		shape.mass = mass;
		shape.height = 1.f;
		shape.width = 1.f;
		shape.depth = 1.f;
		calculateBoxInertia(&shape);
	}

	void calculateBoxInertia(BoxShape* shape) {
		float m = shape->mass;
		float w = shape->width;
		float h = shape->height;
		shape->momentOfInertia = m * (w * w * h * h) / 12;
	}

	void computeForceAndTorque(glm::vec3 force, glm::vec3 point) {
		this->force = force;
		// point is the "arm vector" that goes from the center of mass to the point of force application
		// That is to say {0,0,0} would be the center of mass, {shape.width / 2.f, shape.height / 2.f, shape.depth / 2.f} would be a corner
		torque = glm::cross(force, point);
		//std::cout << glm::to_string(torque) << std::endl;
	}

	void computeLinearAcceleration() {
		glm::vec3 linearAcceleration{ force.x / shape.mass, force.y / shape.mass, force.z / shape.mass };
		linearVelocity += linearAcceleration;
	}

	void computeAngularAcceleration() {
		glm::vec3 angularAcceleration{ torque.x / shape.momentOfInertia, torque.y / shape.momentOfInertia, torque.z / shape.momentOfInertia };
		angularVelocity += angularAcceleration * 30.f;
	}

private:

};
