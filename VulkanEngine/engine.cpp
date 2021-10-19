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


std::vector<GameObject> Engine::gameObjects;
glm::vec3 Engine::lightPos; //probably temporary

Engine::Engine() {
	loadGameObjects();
	renderer.loadDescriptorSets();

	//tell python where to find c++ interaction methods 
	PyImport_AppendInittab("engine", &PythonManager::PyInit_engine);
}

Engine::~Engine() {
	gameObjects.clear();
	AssetManager::clearTextures();
}

void Engine::loadGameObjects() {
	/*AssetManager::loadTexture(device, "models/backpack/diffuse.jpg", "backpack", true);
	AssetManager::loadTexture(device, "textures/apple.jpg", "apple");

	std::shared_ptr<Model> model =
		Model::createModelFromFile(device, "models/backpack/backpack.obj", AssetManager::textures["backpack"]);

	auto gameObj = GameObject::createGameObject("room");
	gameObj.model = model;
	gameObj.transform.translation = { .0f, .0f, 2.5f };
	gameObj.transform.scale = glm::vec3(0.5f);
	gameObjects.push_back(std::move(gameObj));

	std::shared_ptr<Model> model2 =
		Model::createModelFromFile(device, "models/apple.obj", AssetManager::textures["apple"]);
	for (int i = 0; i < 5; i++) {
		auto gameObj2 = GameObject::createGameObject("apple" + std::to_string(i));
		gameObj2.model = model2;
		gameObj2.transform.translation = { 1.f, .0f + i * 0.1f, 2.5f };
		gameObj2.transform.scale = glm::vec3(1.f);
		gameObjects.push_back(std::move(gameObj2));
	}

	lightPos = glm::vec3(0, 1, 3);*/
	AssetManager::loadTexture(device, "textures/apple.jpg", "apple");

	std::shared_ptr<Model> model =
		Model::createModelFromFile(device, "models/apple.obj", AssetManager::textures["apple"]);
	for (int i = 0; i < 5; i++) {
		auto gameObj = GameObject::createGameObject("apple" + std::to_string(i));
		gameObj.model = model;
		gameObj.transform.translation = { 0.f, .0f + i * 0.1f, 0.f };
		gameObj.transform.scale = glm::vec3(1.f);
		gameObjects.push_back(std::move(gameObj));
	}

	lightPos = glm::vec3(0, 1, 3);
}


void Engine::render() {
	glfwPollEvents();
	if (auto commandBuffer = renderer.beginFrame()) {
		renderer.beginSwapChainRenderPass(commandBuffer);
		renderManager.renderGameObjects(commandBuffer, gameObjects, camera, renderer.getUniformBuffersMemory(), renderer.getDescriptorSets());
		renderer.endSwapChainRenderPass(commandBuffer);
		renderer.endFrame();
	}
}

void Engine::update() {
	Engine::physics();
	//PythonManager::runUpdates();
	camera.setProjection(glm::radians(45.f), renderer.getAspectRatio(), 0.1f, 100.f);
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

//handles physics objects in scene
void Engine::physics() {
	for (auto& obj : gameObjects) {
		glm::vec3 gravityAcceleration { 0.f, -0.0003f, 0.f };
		obj.particle.velocity += obj.particle.force(gravityAcceleration);
		obj.transform.translation += obj.particle.velocity;
	}
}

//eventually should handle all shutdown procedures
void Engine::shutdown() {
	window.setWindowShouldClose();
}
