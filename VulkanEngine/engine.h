#pragma once

#include <memory>
#include <vector>
#include <mutex>

#include "window.h"
#include "device.h"
#include "gameObject.h"
#include "renderer.h"
#include "camera.h"
#include "renderManager.h"
#include "descriptorManager.h"
//#include "model.h"

class Engine {
public:
	int width = 800;
	int height = 600;

	static std::vector<GameObject> gameObjects;
	static glm::vec3 lightPos;
	static bool reloadBuffers;
	static std::mutex mtx;

	Engine();
	~Engine();

	//delete copy constructors
	Engine(const Engine&) = delete;
	Engine& operator=(const Engine&) = delete;

	void run();

	void update();
	void render();

	static void addGameObject(std::shared_ptr<Model> model, std::string name);
	static std::string modelToAdd;
	static std::string modelFilepath;
	static std::string modelTexture;
	static std::vector<std::vector<char*>> curImage;
	static std::atomic<bool> takeImage;
private:
	Window window{width, height, "Vulkan"};
	Device device{ window };
	Renderer renderer{ window, device };
	DescriptorManager descriptorManager{ device };
	RenderManager renderManager{ device, renderer.getSwapChainRenderPass() };
    Camera camera{};

	std::vector<VkBuffer> uniformBuffers; //TOOD: rework to use buffer class
	std::vector<VkDeviceMemory> uniformBuffersMemory;
	

	void loadGameObjects();
	void updateBuffers();
	void addModel();
	void getCurrentImage();

	void shutdown();
};

