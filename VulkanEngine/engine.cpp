#include "engine.h"

#include <stdexcept>
#include <iostream>
#include <thread>
#include <math.h>
#include <csignal>

#include "spdlog/spdlog.h"

// logic for python in debug mode
// little bit hacky but should do the trick
#ifdef _DEBUG
#define _DEBUG_WAS_DEFINED
#undef _DEBUG
#endif

#include "python/Python.h"

#ifdef _DEBUG_WAS_DEFINED
#define _DEBUG
#undef _DEBUG_WAS_DEFINED
#endif

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include "glm/glm.hpp"
#include "glm/gtc/constants.hpp"
#include "glm/gtx/string_cast.hpp"
#include "stb_image.h"

#include "renderManager.h"
#include "camera.h"
#include "inputManager.h"
#include "pythonManager.h"
#include "assetManager.h"
#include "constants.h"


std::vector<GameObject> Engine::gameObjects;
glm::vec3 Engine::lightPos; //probably temporary

Engine::Engine() {
	loadGameObjects();

	//tell python where to find c++ interaction methods 
	PyImport_AppendInittab("engine", &PythonManager::PyInit_engine);
}

Engine::~Engine() {
	for (size_t i = 0; i < uniformBuffers.size(); i++) {
        vkDestroyBuffer(device.device(), uniformBuffers[i], nullptr);
        vkFreeMemory(device.device(), uniformBuffersMemory[i], nullptr);
    }
	gameObjects.clear();
	AssetManager::clearTextures();
}



void Engine::loadGameObjects() {
	AssetManager::loadTexture(device, "models/backpack/diffuse.jpg", "backpack", true);
	AssetManager::loadTexture(device, "textures/camel.jpg", "camel");
	AssetManager::loadTexture(device, "textures/heightmap.png", "heightmap");
	AssetManager::loadTexture(device, "textures/apple.jpg", "apple");
	AssetManager::loadTexture(device, "textures/sand.jpg", "sand");
	//std::array<std::string, 6> filepaths;
	//right left top bottom front back
	/*filepaths[0] = "textures/skybox/right.jpg";
	filepaths[1] = "textures/skybox/left.jpg";
	filepaths[2] = "textures/skybox/top.jpg";
	filepaths[3] = "textures/skybox/bottom.jpg";
	filepaths[4] = "textures/skybox/front.jpg";
	filepaths[5] = "textures/skybox/back.jpg";*/
	std::array<std::string, 6> filepaths = {
		"textures/skybox2/right.png",
		"textures/skybox2/left.png",
		"textures/skybox2/top.png",
		"textures/skybox2/bottom.png",
		"textures/skybox2/front.png",
		"textures/skybox2/back.png"
	};

	AssetManager::loadCubeMap(device, filepaths, "skybox");

	std::shared_ptr<Model> model =
		Model::createModelFromFile(device, "models/backpack/backpack.obj", AssetManager::textures["backpack"]);

	auto gameObj = GameObject::createGameObject("room");
	gameObj.model = model;
	gameObj.transform.translation = { .0f, .0f, 2.5f };
	gameObj.transform.scale = glm::vec3(0.5f);
	gameObjects.push_back(std::move(gameObj));

	auto gameObj1 = GameObject::createGameObject("water");
	gameObj1.model = Model::generateMesh(device, 400, 400, AssetManager::textures["skybox"]);
	gameObj1.transform.scale = glm::vec3(0.5f);
	gameObj1.transform.translation = glm::vec3(-75, -5, -75);
	gameObjects.push_back(std::move(gameObj1));

	auto gameObj4 = GameObject::createGameObject("terrain");
	gameObj4.model = //generateMesh(25, 25, AssetManager::textures["sand"], "textures/heightmap.png");
	gameObj4.model = Model::generateTerrain(device);
	gameObj4.transform.translation = glm::vec3(0, 5, 0);
	gameObjects.push_back(std::move(gameObj4));

	std::shared_ptr<Model> model2 =
		Model::createModelFromFile(device, "models/apple.obj", AssetManager::textures["apple"]);
	for (int i = 0; i < 5; i++) {
		auto gameObj2 = GameObject::createGameObject("apple" + std::to_string(i));
		gameObj2.model = model2;
		gameObj2.transform.translation = { 1.f, .0f + i * 0.1f, 2.5f };
		gameObj2.transform.scale = glm::vec3(1.f);
		gameObjects.push_back(std::move(gameObj2));
	}

	std::shared_ptr<Model> model3 =
		Model::createModelFromFile(device, "models/textured_cube.obj", AssetManager::textures["skybox"]);

	auto gameObj3 = GameObject::createGameObject("skybox");
	gameObj3.model = model3;
	gameObjects.push_back(std::move(gameObj3));


	lightPos = glm::vec3(-40, 20, 100);

	uniformBuffers.resize(Engine::gameObjects.size());
    uniformBuffersMemory.resize(Engine::gameObjects.size());
	size_t bufferSize;
    for (size_t i = 0; i < Engine::gameObjects.size(); i++) {
		if (Engine::gameObjects[i].getTag() == "skybox") {
			bufferSize = sizeof(Constants::CubeMapUBO);
		}
		else if (Engine::gameObjects[i].getTag() == "terrain") {
			bufferSize = sizeof(Constants::TesselationUBO);
		}
		else {
			bufferSize = sizeof(Constants::ObjectUBO);
		}

		device.createBuffer(bufferSize,
			VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, 
			VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, 
			uniformBuffers[i], 
			uniformBuffersMemory[i]);

    }

	descriptorManager.createDescriptorPool(static_cast<uint32_t>(gameObjects.size() * 2));
	descriptorManager.createDescriptorSets(static_cast<uint32_t>(gameObjects.size() * 2));


	for (size_t i = 0; i < Engine::gameObjects.size(); i++) {
		if (Engine::gameObjects[i].getTag() == "skybox") {
			bufferSize = sizeof(Constants::CubeMapUBO);
			descriptorManager.updateObjectDescriptorSet(
				gameObjects[i],
				bufferSize,
				DescriptorManager::descriptorSets.objects[i],
				uniformBuffers[i],
				gameObjects[i].model->getTexture()->getImageView());
		}
		else if (Engine::gameObjects[i].getTag() == "terrain") {
			bufferSize = sizeof(Constants::TesselationUBO);
			descriptorManager.updateTerrainDescriptorSet(
				gameObjects[i],
				bufferSize,
				DescriptorManager::descriptorSets.terrain,
				uniformBuffers[i],
				AssetManager::textures["heightmap"]->getImageView(),
				gameObjects[i].model->getTexture()->getImageView());
		}
		else {
			bufferSize = sizeof(Constants::ObjectUBO);
			descriptorManager.updateObjectDescriptorSet(
				gameObjects[i],
				bufferSize,
				DescriptorManager::descriptorSets.objects[i],
				uniformBuffers[i],
				gameObjects[i].model->getTexture()->getImageView());
		}

	}
}


void Engine::render() {
	glfwPollEvents();
	if (auto commandBuffer = renderer.beginFrame()) {
		renderer.beginSwapChainRenderPass(commandBuffer);
		renderManager.renderGameObjects(commandBuffer, gameObjects, camera, uniformBuffersMemory);
		renderer.endSwapChainRenderPass(commandBuffer);
		renderer.endFrame();
	}
}

void Engine::update() {
	//PythonManager::runUpdates();
	camera.setProjection(glm::radians(45.f), renderer.getAspectRatio(), 0.1f, 500.f);
	if (InputManager::keys[GLFW_KEY_W]) {
		camera.moveCamForward(.05f);
	}
	if (InputManager::keys[GLFW_KEY_S]) {
		camera.moveCamBack(.05f);
	}
	if (InputManager::keys[GLFW_KEY_A]) {
		camera.moveCamLeft(.05f);
	}
	if (InputManager::keys[GLFW_KEY_D]) {
		camera.moveCamRight(.05f);
	}
	camera.rotateCamera(InputManager::xoffset, InputManager::yoffset, 0.1f);
	InputManager::xoffset = 0;
	InputManager::yoffset = 0;

	if (InputManager::keys[GLFW_KEY_ESCAPE]) {
		shutdown(); 
	}
}

void Engine::run() {
	PythonManager::initPython();

	//starting values for camera
	InputManager::xoffset = 0;
	InputManager::yoffset = 0;
	camera.rotateCamera(-90, 0, 1);

	double lastTime = glfwGetTime(), timer = lastTime;
	double deltaTime = 0, nowTime = 0;
	int frames = 0, updates = 0;
	const double delta = 1.0 / 120.0;

	while (!window.shouldClose()) {
		//get time
		nowTime = glfwGetTime();
		deltaTime += (nowTime - lastTime) / delta;
		lastTime = nowTime;

		//update at delta
		while (deltaTime >= 1.0) {
			update();
			updates++;
			deltaTime--;
		}

		render();
		frames++;

		//reset and output fps
		if (glfwGetTime() - timer > 1.0) {
			timer++;
			//std::cout << "FPS: " << frames << " Updates:" << updates << std::endl;
			updates = 0, frames = 0;
		}
	}

	vkDeviceWaitIdle(device.device());
}

//eventually should handle all shutdown procedures
void Engine::shutdown() {
	window.setWindowShouldClose();
}
