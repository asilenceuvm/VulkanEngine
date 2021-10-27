#pragma once

#include <memory>
#include <vector>

#include "window.h"
#include "device.h"
#include "gameObject.h"
#include "renderer.h"
#include "camera.h"
#include "renderManager.h"

class Engine {
public:
	int width = 800;
	int height = 600;

	static std::vector<GameObject> gameObjects;
	static glm::vec3 lightPos;

	Engine();
	~Engine();

	//delete copy constructors
	Engine(const Engine&) = delete;
	Engine& operator=(const Engine&) = delete;

	void run();

	void update();
	void render();

	void physics();

	void broadDetectionPhase();
	int testAABBOverlap(std::vector<glm::vec3> box1, std::vector<glm::vec3> box2);
	void narrowDetectionPhase(GameObject* obj1, GameObject* obj2);
	int sphereSphereCollision(glm::vec3 center1, float radius1, glm::vec3 center2, float radius2);
	void decayVecolities();
private:
	Window window{width, height, "Vulkan"};
	Device device{ window };
	Renderer renderer{ window, device };
	RenderManager renderManager{ device, renderer.getSwapChainRenderPass(), renderer.getDescriptorSetLayout() };
    Camera camera{};


	void loadGameObjects();

	void shutdown();
};

