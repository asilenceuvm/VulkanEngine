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
std::string Engine::modelToAdd = "";
std::string Engine::modelFilepath = "";
std::string Engine::modelTexture = "";	
bool Engine::reloadBuffers = false;
std::mutex Engine::mtx; 

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
	AssetManager::clearModels();
	AssetManager::clearTextures();
}


void Engine::addGameObject(std::shared_ptr<Model> model, std::string name) {
	Engine::mtx.lock();
	auto gameObj = GameObject::createGameObject(name);
	gameObj.model = model;
	Engine::gameObjects.push_back(std::move(gameObj));
	//Engine::gameObjects.insert(Engine::gameObjects.end() - 2, std::move(gameObj));
	//auto it = Engine::gameObjects.begin() + Engine::gameObjects.size() - 1;
	//std::rotate(it, it + 1, Engine::gameObjects.end());
	Engine::reloadBuffers = true;
	Engine::mtx.unlock();
}


void Engine::updateBuffers() {
	uniformBuffers.resize(gameObjects.size());
    uniformBuffersMemory.resize(gameObjects.size());
	size_t bufferSize;
    for (size_t i = 0; i < gameObjects.size(); i++) {
		if (gameObjects[i].getTag() == "skybox") {
			bufferSize = sizeof(Constants::CubeMapUBO);
		}
		else if (gameObjects[i].getTag() == "terrain") {
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

	//spdlog::debug("{}", gameObjects.size());

	for (size_t i = 0; i < gameObjects.size(); i++) {
		if (gameObjects[i].getTag() == "skybox") {
			bufferSize = sizeof(Constants::CubeMapUBO);
			descriptorManager.updateObjectDescriptorSet(
				gameObjects[i],
				bufferSize,
				DescriptorManager::descriptorSets.objects[i],
				uniformBuffers[i],
				gameObjects[i].model->getTexture()->getImageView());
		}
		else if (gameObjects[i].getTag() == "terrain") {
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

void Engine::loadGameObjects() {
	AssetManager::loadTexture(device, "models/backpack/diffuse.jpg", "backpack", true);
	AssetManager::loadTexture(device, "textures/camel.jpg", "camel");
	AssetManager::loadTexture(device, "textures/heightmap.png", "heightmap");
	AssetManager::loadTexture(device, "textures/apple.jpg", "apple");
	AssetManager::loadTexture(device, "textures/sand.jpg", "sand");
	std::array<std::string, 6> filepaths = {
		"textures/skybox2/right.png",
		"textures/skybox2/left.png",
		"textures/skybox2/top.png",
		"textures/skybox2/bottom.png",
		"textures/skybox2/front.png",
		"textures/skybox2/back.png"
	};
	AssetManager::loadCubeMap(device, filepaths, "skybox");

	AssetManager::loadModel(device, "backpack", "models/backpack/backpack.obj", "backpack"); 
	AssetManager::loadModel(device, "apple", "models/apple.obj", "apple"); 
	AssetManager::loadModel(device, "skybox", "models/textured_cube.obj", "skybox"); 

	auto gameObj = GameObject::createGameObject("room");
	gameObj.model = AssetManager::models["backpack"];
	gameObj.transform.translation = { .0f, .0f, 2.5f };
	gameObj.transform.scale = glm::vec3(0.5f);
	gameObjects.push_back(std::move(gameObj));

	auto gameObj1 = GameObject::createGameObject("water");
	gameObj1.model = Model::generateMesh(device, 400, 400, AssetManager::textures["skybox"]);
	gameObj1.transform.scale = glm::vec3(0.5f);
	gameObj1.transform.translation = glm::vec3(-75, -5, -75);
	gameObjects.push_back(std::move(gameObj1));

	auto gameObj4 = GameObject::createGameObject("terrain");
	gameObj4.model = Model::generateTerrain(device, 64, 1.f);
	gameObj4.transform.translation = glm::vec3(0, 4, 0);
	gameObjects.push_back(std::move(gameObj4));


	for (int i = 0; i < 5; i++) {
		auto gameObj2 = GameObject::createGameObject("apple" + std::to_string(i));
		gameObj2.model = AssetManager::models["apple"];
		gameObj2.transform.translation = { 1.f, .0f + i * 0.1f, 2.5f };
		gameObj2.transform.scale = glm::vec3(1.f);
		gameObjects.push_back(std::move(gameObj2));
	}


	auto gameObj3 = GameObject::createGameObject("skybox");
	gameObj3.model = AssetManager::models["skybox"];
	gameObjects.push_back(std::move(gameObj3));


	lightPos = glm::vec3(-40, 20, 100);

	updateBuffers();
}


void Engine::render() {
	glfwPollEvents();
	auto commandBuffer = renderer.beginFrame();
	if (commandBuffer && !reloadBuffers) {
		//Engine::mtx.lock();
		//if (reloadBuffers == true) {
			//updateBuffers();
			//reloadBuffers = false;
		//}
		//else {
			renderer.beginSwapChainRenderPass(commandBuffer);
			renderManager.renderGameObjects(commandBuffer, gameObjects, camera, uniformBuffersMemory);
			renderer.endSwapChainRenderPass(commandBuffer);
			renderer.endFrame();
		//}
		//Engine::mtx.unlock();
	}
	if (reloadBuffers == true) {
		Engine::mtx.lock();
		updateBuffers();
		reloadBuffers = false;
		Engine::mtx.unlock();
		//updateBuffers();
	}
}

void Engine::update() {
	//PythonManager::runUpdates();
	camera.setProjection(glm::radians(45.f), renderer.getAspectRatio(), 0.1f, 1000.f);
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

	if (modelToAdd != "" && modelFilepath != "" && modelTexture != "") {
		AssetManager::loadTexture(device, modelTexture, modelTexture);
		AssetManager::loadModel(device, modelToAdd, modelFilepath, modelTexture); 
		modelToAdd = "";
		modelFilepath = "";
		modelTexture = "";
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
