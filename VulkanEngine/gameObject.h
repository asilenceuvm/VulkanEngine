#pragma once

#include <memory>
#include <iostream>

#include "glm/gtc/matrix_transform.hpp"

#include "model.h"
#include "particle.h"
#include "gjk.h"

struct TransformComponent {
	glm::vec3 translation{};
	glm::vec3 scale{ 1.f, 1.f, 1.f };
	glm::vec3 rotation{ 1.f, 0.f, 0.f };

	glm::mat4 mat4() {
		glm::mat4 transform = glm::mat4(1.0f);
		transform = glm::translate(transform, translation);
		transform = glm::rotate(transform, glm::radians(rotation.x), glm::vec3(1.f, 0.f, 0.f)); 
		transform = glm::rotate(transform, glm::radians(rotation.y), glm::vec3(0.f, 1.f, 0.f)); 
		transform = glm::rotate(transform, glm::radians(rotation.z), glm::vec3(0.f, 0.f, 1.f)); 
		transform = glm::scale(transform, scale);

		return transform;
	};
};

class GameObject {
public:
	using id_t = unsigned int;

	std::shared_ptr<Model> model{};
	glm::vec3 color{};
	TransformComponent transform{};

	Particle particle = Particle(1, glm::vec3{ 0.f, 0.f, 0.f }, glm::vec3{ 0.f, 0.f, 0.f });

	MeshCollider currentWorldSpaceCollider;

	static GameObject createGameObject(std::string tag) {
		static id_t currentId = 0;
		return GameObject{ currentId++, tag };
	}

	GameObject(const GameObject&) = delete;
	GameObject& operator=(const GameObject&) = delete;
	GameObject(GameObject&&) = default;
	GameObject& operator=(GameObject&&) = default;

	void calculateCurrentWorldSpaceVectors() {
		std::vector<glm::vec3> worldSpaceColliderVectors;
		for (auto& colliderVector : particle.colliderVectors) // access by reference to avoid copying
		{
			worldSpaceColliderVectors.push_back(glm::vec3{ colliderVector.x + transform.translation.x, colliderVector.y + transform.translation.y, colliderVector.z + transform.translation.z });
		}
		currentWorldSpaceCollider.SetVertices(worldSpaceColliderVectors);
	}

	id_t getId() {
		return id;
	}
	std::string getTag() const {
		return tag;
	}

private:
	id_t id;
	std::string tag;

	GameObject(id_t objId, std::string objTag) : id{ objId }, tag{ objTag } {}
};

