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
	RenderManager renderManager{ device, renderer.getSwapChainRenderPass() };
    Camera camera{};

	void loadGameObjects();

	void shutdown();
};

