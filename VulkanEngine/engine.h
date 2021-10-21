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
private:
	Window window{width, height, "Vulkan"};
	Device device{ window };
	Renderer renderer{ window, device };
	RenderManager renderManager{ device, renderer.getSwapChainRenderPass(), renderer.getDescriptorSetLayout() };
    Camera camera{};


	void loadGameObjects();

	void shutdown();

	//temp
	std::unique_ptr<Model> generateMesh(int length, int width, std::shared_ptr<Texture> texture);
};

