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

#include "renderManager.h"
#include "camera.h"
#include "inputManager.h"
#include "pythonManager.h"


std::vector<GameObject> Engine::gameObjects;

Engine::Engine() {
	loadGameObjects();

	//tell python where to find c++ interaction methods 
	PyImport_AppendInittab("engine", &PythonManager::PyInit_engine);
}

Engine::~Engine() {
	gameObjects.clear();
}

//temp
std::unique_ptr<Model> createCubeModel(Device& device, glm::vec3 offset) {
	Model::Geometry modelGeometry{};
	modelGeometry.vertices = {
		// left face (white)
		 {{-.5f, -.5f, -.5f}, {.9f, .9f, .9f}},
		 {{-.5f, .5f, .5f}, {.9f, .9f, .9f}},
		 {{-.5f, -.5f, .5f}, {.9f, .9f, .9f}},
		 {{-.5f, .5f, -.5f}, {.9f, .9f, .9f}},

		 // right face (yellow)
		 {{.5f, -.5f, -.5f}, {.8f, .8f, .1f}},
		 {{.5f, .5f, .5f}, {.8f, .8f, .1f}},
		 {{.5f, -.5f, .5f}, {.8f, .8f, .1f}},
		 {{.5f, .5f, -.5f}, {.8f, .8f, .1f}},

		 // top face (orange, remember y axis points down)
		 {{-.5f, -.5f, -.5f}, {0.8f, 0.1f, 0.8f}},
		 {{.5f, -.5f, .5f},   {0.8f, 0.1f, 0.8f}},
		 {{-.5f, -.5f, .5f},  {0.8f, 0.1f, 0.8f}},
		 {{.5f, -.5f, -.5f},  {0.8f, 0.1f, 0.8f}},

		 // bottom face (red)
		 {{-.5f, .5f, -.5f}, {.8f, .1f, .1f}},
		 {{.5f, .5f, .5f}, {.8f, .1f, .1f}},
		 {{-.5f, .5f, .5f}, {.8f, .1f, .1f}},
		 {{.5f, .5f, -.5f}, {.8f, .1f, .1f}},

		 // nose face (blue)
		 {{-.5f, -.5f, 0.5f}, {.1f, .1f, .8f}},
		 {{.5f, .5f, 0.5f}, {.1f, .1f, .8f}},
		 {{-.5f, .5f, 0.5f}, {.1f, .1f, .8f}},
		 {{.5f, -.5f, 0.5f}, {.1f, .1f, .8f}},

		 // tail face (green)
		 {{-.5f, -.5f, -0.5f}, {.1f, .8f, .1f}},
		 {{.5f, .5f, -0.5f}, {.1f, .8f, .1f}},
		 {{-.5f, .5f, -0.5f}, {.1f, .8f, .1f}},
		 {{.5f, -.5f, -0.5f}, {.1f, .8f, .1f}},

	};
	for (auto& v : modelGeometry.vertices) {
		v.position += offset;
	}

	modelGeometry.indices = {0,  1,  2,  0,  3,  1,  4,  5,  6,  4,  7,  5,  8,  9,  10, 8,  11, 9,
                          12, 13, 14, 12, 15, 13, 16, 17, 18, 16, 19, 17, 20, 21, 22, 20, 23, 21};
	return std::make_unique<Model>(device, modelGeometry);
}

void Engine::loadGameObjects() {
	std::shared_ptr<Model> model = createCubeModel(device, {.0f, .0f, .0f});
	for (int i = 0; i < 10; i++) {
		for (int j = 0; j < 10; j++) {
			auto cube = GameObject::createGameObject("cube" + std::to_string(i) + std::to_string(j));
			cube.model = model;
			cube.transform.translation = { .2f + (i * .2f), .2f + (j * .2f), .5f };
			cube.transform.scale = { .1f, .1f, .1f };
			cube.transform.rotation = { 0.f, 0.5f, 0.f };
			gameObjects.push_back(std::move(cube));
		}
	}
}


void Engine::render() {
	glfwPollEvents();
	if (auto commandBuffer = renderer.beginFrame()) {
		renderer.beginSwapChainRenderPass(commandBuffer);
		renderManager.renderGameObjects(commandBuffer, gameObjects, camera);
		renderer.endSwapChainRenderPass(commandBuffer);
		renderer.endFrame();
	}
}

void Engine::update() {
	//PythonManager::runUpdates(); commented out for testing
	camera.setProjection(glm::radians(45.f),  renderer.getAspectRatio(), 0.1f, 10.f);
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
