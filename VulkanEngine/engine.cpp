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
	/*AssetManager::loadTexture(device, "textures/apple.jpg", "apple");

	std::shared_ptr<Model> model =
		Model::createModelFromFile(device, "models/apple.obj", AssetManager::textures["apple"]);*/

	AssetManager::loadTexture(device, "textures/white.png", "white");

	std::shared_ptr<Model> model =
		Model::createModelFromFile(device, "models/textured_cube.obj", AssetManager::textures["white"]);

	std::shared_ptr<Model> model2 =
		Model::createModelFromFile(device, "models/misha.obj", AssetManager::textures["white"]);

	std::shared_ptr<Model> model3 =
		Model::createModelFromFile(device, "models/owl.obj", AssetManager::textures["white"]);

	/*auto gameObj4 = GameObject::createGameObject("apple" + std::to_string(0));
	gameObj4.model = model;
	gameObj4.transform.translation = { 0.f, 0.f, 0.f };
	gameObj4.transform.scale = glm::vec3(1.f);
	gameObj4.particle.linearVelocity = glm::vec3(0.f, 0.f, 0.f);
	gameObj4.particle.angularVelocity = glm::vec3(0.f, 0.f, 0.f);
	gameObjects.push_back(std::move(gameObj4));

	auto gameObj2 = GameObject::createGameObject("apple" + std::to_string(0));
	gameObj2.model = model;
	gameObj2.transform.translation = { -1.f, -0.05f, 0.f };
	gameObj2.transform.scale = glm::vec3(1.f);
	gameObj2.particle.linearVelocity = glm::vec3(0.0075f, 0.f, 0.f);
	gameObj2.particle.angularVelocity = glm::vec3(0.f, 3.f, 0.f);
	gameObjects.push_back(std::move(gameObj2));*/

	auto gameObjCube = GameObject::createGameObject("cube" + std::to_string(0));
	gameObjCube.model = model;
	gameObjCube.transform.translation = { 0.f, 0.f, 0.f };
	gameObjCube.transform.scale = glm::vec3(0.1f);
	gameObjCube.particle.linearVelocity = glm::vec3(0.f, 0.f, 0.f);
	gameObjCube.particle.angularVelocity = glm::vec3(0.f, 0.f, 0.f);
	gameObjects.push_back(std::move(gameObjCube));

	auto gameObjPlayable = GameObject::createGameObject("player_cube");
	gameObjPlayable.model = model3;
	gameObjPlayable.transform.translation = { -0.5f, 0.5f, 0.f };
	gameObjPlayable.transform.scale = glm::vec3(0.1f);
	gameObjPlayable.particle.linearVelocity = glm::vec3(0.f, 0.f, 0.f);
	gameObjPlayable.particle.angularVelocity = glm::vec3(0.f, 0.5f, 0.f);
	gameObjects.push_back(std::move(gameObjPlayable));

	auto gameObjCat = GameObject::createGameObject("cat" + std::to_string(0));
	gameObjCat.model = model2;
	gameObjCat.transform.translation = { 0.f, 0.5f, 0.f };
	gameObjCat.transform.scale = glm::vec3(0.1f);
	gameObjCat.particle.linearVelocity = glm::vec3(0.f, 0.f, 0.f);
	gameObjCat.particle.angularVelocity = glm::vec3(0.f, -1.25f, 0.f);
	gameObjects.push_back(std::move(gameObjCat));

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

	// Apple velocity/acceleration changing
	/*for (auto& obj : gameObjects) {
		if (obj.getTag() == "player_apple") {
			obj.particle.linearVelocity = glm::vec3{ 0.f, 0.f, 0.f };
		}
	}*/
	if (InputManager::keys[GLFW_KEY_T]) {
		for (auto& obj : gameObjects) {
			if (obj.getTag() == "player_cube") {
				obj.particle.linearVelocity += glm::vec3{0.f, 0.0001f, 0.f};
			}
		}
	}
	if (InputManager::keys[GLFW_KEY_G]) {
		for (auto& obj : gameObjects) {
			if (obj.getTag() == "player_cube") {
				obj.particle.linearVelocity += glm::vec3{ 0.f, -0.0001f, 0.f };
			}
		}
	}
	if (InputManager::keys[GLFW_KEY_F]) {
		for (auto& obj : gameObjects) {
			if (obj.getTag() == "player_cube") {
				obj.particle.linearVelocity += glm::vec3{ -0.0001f, 0.f, 0.f };
			}
		}
	}
	if (InputManager::keys[GLFW_KEY_H]) {
		for (auto& obj : gameObjects) {
			if (obj.getTag() == "player_cube") {
				obj.particle.linearVelocity += glm::vec3{ 0.0001f, 0.f, 0.f };
			}
		}
	}

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

// Handles physics objects in scene
void Engine::physics() {
	// Upkeep
	//decayVecolities();
	
	// Check for collisions
	broadDetectionPhase();
	for (auto& obj : gameObjects) {
		/* 1) --- Calculate External Forces--- 
		 * Forces such as gravity or objects colliding.
		 */
		// Gravity vector
		glm::vec3 gravityAcceleration{ 0.f, 0.f, 0.f };//-0.000005f, 0.f };
		// Location of force (Top)
		glm::vec3 point{ 0.f, obj.particle.shape.width / 2.f, 0.f };
		glm::vec3 forceExternal = gravityAcceleration;
		/* 2) --- Calculate Constraint Forces--- 
		 * Forces that keep an object where it should be. For instance
		 * this will keep joints together, stop an object from clipping
		 * into another, etc.
		 */
		glm::vec3 forceConstraint{ 0.f, 0.f, 0.f };
		/* 3) --- Apply Forces and Simulate Motion--- 
		 * Finally apply the two forces for each object, changing
		 * based on the mass of the object.
		 */
		glm::vec3 force = forceExternal + forceConstraint;
		obj.particle.computeForceAndTorque(gravityAcceleration, point * 50.f);
		// Compute the object's accelerations
		obj.particle.computeLinearAcceleration();
		obj.particle.computeAngularAcceleration();
		// Translate and rotate the object
		obj.transform.translation += obj.particle.linearVelocity;
		obj.transform.rotation += obj.particle.angularVelocity;
	}
}

// Detect potential collisions using axis-aligend bounding boxes (AABB)
void Engine::broadDetectionPhase() {
	for (auto& obj1 : gameObjects) {
		for (auto& obj2 : gameObjects) {
			// Ensure the objects aren't the same object
			if (obj1.getId() != obj2.getId()) {
				std::vector box1{ obj1.transform.translation - obj1.particle.shape.width, obj1.transform.translation + obj1.particle.shape.width };
				std::vector box2{ obj2.transform.translation - obj2.particle.shape.width, obj2.transform.translation + obj2.particle.shape.width };
				if (Engine::testAABBOverlap(box1, box2)) {
					Engine::narrowDetectionPhase(&obj1, &obj2);
				}
			}
		}
	}
}

// Find deltas for x, y, and z by subtracting max from min
// TODO: Make more efficient by implementing Sort and Sweep Algorithm (Used by Bullet)
int Engine::testAABBOverlap(std::vector<glm::vec3> box1, std::vector<glm::vec3> box2) {
	float d1x = box1[0].x - box2[1].x;
	float d1y = box1[0].y - box2[1].y;
	float d1z = box1[0].z - box2[1].z;
	float d2x = box2[0].x - box1[1].x;
	float d2y = box2[0].y - box1[1].y;
	float d2z = box2[0].z - box1[1].z;

	//std::cout << box1[0].x << "," << box1[0].y << "," << box1[0].z << std::endl;
	//std::cout << box1[1].x << "," << box1[1].y << "," << box1[1].z << std::endl;

	// Check for overlapping bounding boxes
	if (d1x > 0.f || d1y > 0.f || d1z > 0.f) {
		// No collision found
		return 0;
	}
	if (d2x > 0.f || d2y > 0.f || d2z > 0.f) {
		// No collision found
		return 0;
	}
	// Possible collision found
	return 1;
}

void Engine::narrowDetectionPhase(GameObject* obj1, GameObject* obj2) {
	// If both objects are spheres
	//int colliding = Engine::sphereSphereCollision(obj1->transform.translation, 0.05f, obj2->transform.translation, 0.05f);
	// Otherwise, use GJK
	obj1->calculateCurrentWorldSpaceVectors();
	obj2->calculateCurrentWorldSpaceVectors();
	CollisionInfo info1 = GJK(&obj1->currentWorldSpaceCollider, &obj2->currentWorldSpaceCollider);
	CollisionInfo info2 = GJK(&obj2->currentWorldSpaceCollider, &obj1->currentWorldSpaceCollider);
	//std::cout << colliding << std::endl;
	if (info1.colliding) {
		// Collision Details
		//std::cout << obj1->getTag() << " -> " << obj2->getTag() << std::endl;
		//std::cout << "(" << info1.collisionCentroid.x << "," << info1.collisionCentroid.y << "," << info1.collisionCentroid.z << ")" << std::endl;
		//std::cout << "(" << info2.collisionCentroid.x << "," << info2.collisionCentroid.y << "," << info2.collisionCentroid.z << ")" << std::endl;
		//std::cout << "(" << std::abs(info1.collisionCentroid.x)-std::abs(info2.collisionCentroid.x) << "," << std::abs(info1.collisionCentroid.y) - std::abs(info2.collisionCentroid.y) << "," << std::abs(info1.collisionCentroid.z) - std::abs(info2.collisionCentroid.z) << ")" << std::endl;
		//obj1->particle.linearVelocity = glm::normalize(glm::vec3{ std::abs(info1.collisionCentroid.x) - std::abs(info2.collisionCentroid.x), std::abs(info1.collisionCentroid.y) - std::abs(info2.collisionCentroid.y), std::abs(info1.collisionCentroid.z) - std::abs(info2.collisionCentroid.z) }) / 100.0f;
		
		// So we know if we add the velocity of the other guy it will move it in the expected direction, HOWEVER, this does not
		// deal with the constraint forces that should also be added so the objects stop having collisions (ie. the sticky issue)

		// Regular force (Add/Subtract velocities)
		obj1->particle.linearVelocity += ((obj2->particle.linearVelocity / 2.0f) - (obj1->particle.linearVelocity / 2.0f));

		// Constraint force (Make the objects stop touching so we don't have a bunch of collisions and weird velocity bugs)
		// The easiest way to guarentee objects stop touching is to simply move them in the opposite direction of eachother,
		// let's try to do this by calculating the normal vector from the centroids of both objects (approximately where they collided).
		// TODO: This normal seems to go in the wrong direction, maybe we have to use the normal tangent instead?
		obj1->particle.linearVelocity += glm::normalize(glm::vec3{ std::abs(info1.collisionCentroid.x) - std::abs(info2.collisionCentroid.x), std::abs(info1.collisionCentroid.y) - std::abs(info2.collisionCentroid.y), std::abs(info1.collisionCentroid.z) - std::abs(info2.collisionCentroid.z) }) / 10000.0f;

		/*	glm::vec3 normalizedDirection{ obj1->transform.translation.x - obj2->transform.translation.x, obj1->transform.translation.y - obj2->transform.translation.y, obj1->transform.translation.z - obj2->transform.translation.z };
		normalizedDirection = glm::normalize(normalizedDirection);
		// Compute forces and torques for colliding objects
		glm::vec3 computedForceObj1{ obj2->particle.linearVelocity.x - obj1->particle.linearVelocity.x, obj2->particle.linearVelocity.y - obj1->particle.linearVelocity.y, obj2->particle.linearVelocity.z - obj1->particle.linearVelocity.z };
		glm::vec3 computedForceObj2{ obj1->particle.linearVelocity.x - obj2->particle.linearVelocity.x, obj1->particle.linearVelocity.y - obj2->particle.linearVelocity.y, obj1->particle.linearVelocity.z - obj2->particle.linearVelocity.z };
		//std::cout << "---- Debug Information ----" << std::endl;
		//std::cout << "Computed Forces (Pre-normalization)" << std::endl;
		//std::cout << computedForceObj1.x << "," << computedForceObj1.y << "," << computedForceObj1.z << std::endl;
		//std::cout << computedForceObj2.x << "," << computedForceObj2.y << "," << computedForceObj2.z << std::endl;
		computedForceObj1 = (computedForceObj1.x + computedForceObj1.y + computedForceObj1.z)/3.f * normalizedDirection;
		computedForceObj2 = (computedForceObj2.x + computedForceObj2.y + computedForceObj2.z)/3.f * normalizedDirection;
		//std::cout << "Linear Velocities" << std::endl;
		//std::cout << obj1->particle.linearVelocity.x << "," << obj1->particle.linearVelocity.y << "," << obj1->particle.linearVelocity.z << std::endl;
		//std::cout << obj2->particle.linearVelocity.x << "," << obj2->particle.linearVelocity.y << "," << obj2->particle.linearVelocity.z << std::endl;
		//std::cout << "Normal Direction" << std::endl;
		//std::cout << normalizedDirection.x << "," << normalizedDirection.y << "," << normalizedDirection.z << std::endl;
		//std::cout << "Computed Forces (Post-normalization)" << std::endl;
		//std::cout << computedForceObj1.x << "," << computedForceObj1.y << "," << computedForceObj1.z << std::endl;
		//std::cout << computedForceObj2.x << "," << computedForceObj2.y << "," << computedForceObj2.z << std::endl;
		obj1->particle.computeForceAndTorque(computedForceObj1, glm::vec3{ -0.05f,-0.05f,0.f });
		obj2->particle.computeForceAndTorque(computedForceObj2, glm::vec3{ 0.05f,0.05f,0.f });
		// Compute new accelerations after collision
		obj1->particle.computeLinearAcceleration();
		obj1->particle.computeAngularAcceleration();
		obj2->particle.computeLinearAcceleration();
		obj2->particle.computeAngularAcceleration();
		// Transform and rotate objects
		obj1->transform.translation += obj1->particle.linearVelocity;
		obj1->transform.rotation += obj1->particle.angularVelocity;
		obj2->transform.translation += obj2->particle.linearVelocity;
		obj2->transform.rotation += obj2->particle.angularVelocity;*/
	}
}

int Engine::sphereSphereCollision(glm::vec3 center1, float radius1, glm::vec3 center2, float radius2) {
	float x = center1.x - center2.x;
	float y = center1.y - center2.y;
	float z = center1.z - center2.z;
	float squaredCenterDistance = x * x + y * y + z * z;
	float r = radius1 + radius2;
	float rSquared = r * r;
	if (squaredCenterDistance <= rSquared) {
		return 1;
	}
	return 0;
}

void Engine::decayVecolities() {
	for (auto& obj : gameObjects) {
		obj.particle.linearVelocity = obj.particle.linearVelocity - (obj.particle.linearVelocity * obj.particle.drag);
	}
}

//eventually should handle all shutdown procedures
void Engine::shutdown() {
	window.setWindowShouldClose();
}
