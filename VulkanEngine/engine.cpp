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

	std::shared_ptr<Model> model4 =
		Model::createModelFromFile(device, "models/apple.obj", AssetManager::textures["apple"]);

	AssetManager::loadTexture(device, "textures/white.png", "white");

	std::shared_ptr<Model> model =
		Model::createModelFromFile(device, "models/textured_cube.obj", AssetManager::textures["white"]);

	std::shared_ptr<Model> model2 =
		Model::createModelFromFile(device, "models/pirate-ship.obj", AssetManager::textures["white"]);

	std::shared_ptr<Model> model3 =
		Model::createModelFromFile(device, "models/sphere.obj", AssetManager::textures["white"]);

	std::shared_ptr<Model> floorModel =
		Model::createModelFromFile(device, "models/floor.obj", AssetManager::textures["white"]);

	//std::shared_ptr<Model> model3 =
	//	Model::createModelFromFile(device, "models/minecraft-steve.obj", AssetManager::textures["white"]);

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

	auto gameObj1 = GameObject::createGameObject("1");
	gameObj1.model = model;
	gameObj1.transform.translation = { 12.f, 0.5f, 0.f };
	gameObj1.transform.scale = glm::vec3(0.15f);
	gameObj1.particle.linearVelocity = glm::vec3(0.f, 0.f, 0.f);
	gameObj1.particle.angularVelocity = glm::vec3(0.f, 0.f, 0.f);
	gameObj1.particle.gravityAcceleration = glm::vec3(0.f, 0.f, 0.f);
	gameObjects.push_back(std::move(gameObj1));

	auto gameObj3 = GameObject::createGameObject("3");
	gameObj3.model = model;
	gameObj3.transform.translation = { 12.f, 1.5f, 0.f };
	gameObj3.transform.scale = glm::vec3(0.15f);
	gameObj3.particle.linearVelocity = glm::vec3(0.f, 0.f, 0.f);
	gameObj3.particle.angularVelocity = glm::vec3(0.f, 0.f, 0.f);
	gameObj3.particle.gravityAcceleration = glm::vec3(0.f, 0.f, 0.f);
	gameObjects.push_back(std::move(gameObj3));

	auto gameObj2 = GameObject::createGameObject("2");
	gameObj2.model = model;
	gameObj2.transform.translation = { 12.f, -0.5f, 0.f };
	gameObj2.transform.scale = glm::vec3(0.15f);
	gameObj2.particle.linearVelocity = glm::vec3(0.f, 0.f, 0.f);
	gameObj2.particle.angularVelocity = glm::vec3(0.f, 0.f, 0.f);
	gameObj2.particle.position_lock = true;
	gameObjects.push_back(std::move(gameObj2));

	auto gameObjPlayable = GameObject::createGameObject("player_cube");
	gameObjPlayable.model = model4;
	gameObjPlayable.transform.translation = { 10.f, 0.f, -2.f };
	gameObjPlayable.transform.scale = glm::vec3(0.15f);
	gameObjPlayable.particle.angularVelocity = glm::vec3(0.f, 0.f, 0.f);
	gameObjects.push_back(std::move(gameObjPlayable));

	auto gameObjPlayable2 = GameObject::createGameObject("cube");
	gameObjPlayable2.model = model4;
	gameObjPlayable2.transform.translation = { 11.f, 0.2f, -2.f };
	gameObjPlayable2.transform.scale = glm::vec3(0.15f);
	gameObjPlayable2.particle.linearVelocity = glm::vec3(0.f, 0.f, 0.f);
	gameObjPlayable2.particle.angularVelocity = glm::vec3(0.f, 0.f, 0.f);
	gameObjects.push_back(std::move(gameObjPlayable2));

	// Demo 1 - Billard Balls
	// White ball
	auto gameObjBall = GameObject::createGameObject("whiteball");
	gameObjBall.model = model3;
	gameObjBall.transform.translation = { 0.f, 2.f, -2.f };
	gameObjBall.transform.scale = glm::vec3(0.35f);
	gameObjBall.particle.linearVelocity = glm::vec3(0.f, 0.f, 0.f);
	gameObjBall.particle.angularVelocity = glm::vec3(0.f, 0.f, 0.f);
	gameObjBall.particle.gravityAcceleration = glm::vec3(0.f, 0.f, 0.f);
	gameObjects.push_back(std::move(gameObjBall));
	// Stack
	for (int i = 0; i < 5; i++) {
		for (int j = 5; j > i; j--) {
			auto gameObjBall = GameObject::createGameObject("ball" + std::to_string(0));
			gameObjBall.model = model3;
			gameObjBall.transform.translation = { j * 0.31f + i * 0.31f - 1.615f, j * 0.3f - (i * 0.3f) - 2.2f, -2.f };
			gameObjBall.transform.scale = glm::vec3(0.35f);
			gameObjBall.particle.linearVelocity = glm::vec3(0.f, 0.f, 0.f);
			gameObjBall.particle.angularVelocity = glm::vec3(0.f, 0.f, 0.f);
			gameObjBall.particle.gravityAcceleration = glm::vec3(0.f, 0.f, 0.f);
			gameObjects.push_back(std::move(gameObjBall));
		}
	}

	// Demo 2 - Cannonball
	auto gameObjCannonball = GameObject::createGameObject("cannonball");
	gameObjCannonball.model = model2;
	gameObjCannonball.transform.translation = { 5.f, -0.75f, -2.f };
	gameObjCannonball.transform.scale = glm::vec3(0.35f);
	gameObjCannonball.particle.linearVelocity = glm::vec3(0.f, 0.f, 0.f);
	gameObjCannonball.particle.angularVelocity = glm::vec3(0.f, 0.f, 0.f);
	gameObjCannonball.particle.gravityAcceleration = glm::vec3(0.f, 0.f, 0.f);
	gameObjects.push_back(std::move(gameObjCannonball));


	auto gameObjFloor = GameObject::createGameObject("floor");
	gameObjFloor.model = floorModel;
	gameObjFloor.transform.translation = { 6.5f, -1.f, -2.f };
	gameObjFloor.transform.rotation = { 0.f, 0.f, 0.f };
	gameObjFloor.transform.scale = glm::vec3(0.1f);
	gameObjFloor.particle.position_lock = true;
	gameObjects.push_back(std::move(gameObjFloor));

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
	if (InputManager::keys[GLFW_KEY_1]) {
		// Demo 1 - Billard Balls
		for (auto& obj : gameObjects) {
			if (obj.getTag() == "whiteball") {
				obj.particle.linearVelocity = glm::vec3(0.f, -0.012f, 0.f);
				obj.particle.gravityAcceleration = glm::vec3(0.f, 0.f, -0.000005f);
			}
			if (obj.getTag().find("ball") == 0) {
				obj.particle.gravityAcceleration = glm::vec3(0.f, 0.f, -0.000005f);
			}
		}
	}
	if (InputManager::keys[GLFW_KEY_2]) {
		// Demo 2 - Cannonball pt 1
		for (auto& obj : gameObjects) {
			if (obj.getTag() == "cannonball") {
				obj.particle.linearVelocity = glm::vec3(0.009f, 0.014f, 0.f);
				obj.particle.gravityAcceleration = glm::vec3(0.f, -0.0001f, 0.f);
				obj.particle.angularVelocity = glm::vec3(0.f, -1.125f/2.f, -1.125f);
			}
		}
	}
	if (InputManager::keys[GLFW_KEY_3]) {
		// Demo 2 - Cannonball pt 2
		for (auto& obj : gameObjects) {
			if (obj.getTag() == "cannonball") {
				obj.particle.linearVelocity = glm::vec3(-0.009f, 0.014f, 0.f);
				obj.particle.gravityAcceleration = glm::vec3(0.f, -0.0001f, 0.f);
				obj.particle.angularVelocity = glm::vec3(0.f, -1.125f / 2.f, -1.125f);
			}
		}
	}
	if (InputManager::keys[GLFW_KEY_4]) {
		// Demo 3 - Drop pt 1
		for (auto& obj : gameObjects) {
			if (obj.getTag() == "1") {
				obj.particle.gravityAcceleration = glm::vec3(0.f, -0.00005f, 0.f);
			}
		}
	}
	if (InputManager::keys[GLFW_KEY_5]) {
		// Demo 3 - Drop pt 2
		for (auto& obj : gameObjects) {
			if (obj.getTag() == "3") {
				obj.particle.gravityAcceleration = glm::vec3(0.f, -0.00005f, 0.f);
			}
		}
	}
	// Get Debug Info
	if (InputManager::keys[GLFW_KEY_R]) {
		std::cout << "Debug Positions" << std::endl;
		for (auto& obj : gameObjects) {
			std::cout << obj.getTag() << " Vertices" << std::endl;
			for (auto& colVector : obj.currentWorldSpaceCollider.GetVertices()) // access by reference to avoid copying
			{
				std::cout << colVector.x << "," << colVector.y << "," << colVector.z << std::endl;
			}
		}
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
		if (obj.particle.position_lock) {
			obj.particle.linearVelocity = glm::vec3{0,0,0};
		}
		else {
			/* 1) --- Calculate External Forces--- 
			 * Forces such as gravity or objects colliding.
			 */
			// Location of force (Top)
			glm::vec3 point{ 0.f, obj.particle.shape.width / 2.f, 0.f };
			glm::vec3 forceExternal = obj.particle.gravityAcceleration;
			/* 2) --- Calculate Constraint Forces--- 
			 * Forces that keep an object where it should be. For instance
			 * this will keep joints together, stop an object from clipping
			 * into another, etc.
			 */
			//glm::vec3 forceConstraint{ 0.f, 0.f, 0.f };
			/* 3) --- Apply Forces and Simulate Motion--- 
			 * Finally apply the two forces for each object, changing
			 * based on the mass of the object.
			 */
			glm::vec3 force = forceExternal;
			obj.particle.computeForceAndTorque(force, point);
			// Compute the object's accelerations
			obj.particle.computeLinearAcceleration();
			obj.particle.computeAngularAcceleration();
			// Translate and rotate the object
			if (obj.getTag() == "cannonball" && obj.transform.translation.y < -0.75f) {
				obj.particle.angularVelocity = glm::vec3(0.f,0.f,0.f);
				obj.particle.linearVelocity = glm::vec3(0.f, 0.f, 0.f);
				obj.particle.gravityAcceleration = glm::vec3(0.f, 0.f, 0.f);
				obj.transform.translation = glm::vec3(obj.transform.translation.x, -0.75f, obj.transform.translation.z);
			}
			obj.transform.translation += obj.particle.linearVelocity;
			obj.transform.rotation += obj.particle.angularVelocity;
		}
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

		// Setup
		glm::vec3 force{ 0.f, 0.f, 0.f };
		glm::vec3 point{ info1.collisionCentroid.x, info1.collisionCentroid.y, info1.collisionCentroid.z };

		// Regular force (Add/Subtract velocities)
		force += ((obj2->particle.linearVelocity / 2.0f) - (obj1->particle.linearVelocity / 2.0f));

		//std::cout << constraint.x << "," << constraint.y << "," << constraint.z << std::endl;
		// 
		// Constraint force (Make the objects stop touching so we don't have a bunch of collisions and weird velocity bugs)
		// The easiest way to guarentee objects stop touching is to simply move them in the opposite direction of eachother,
		// let's try to do this by calculating the normal vector from the centroids of both objects (approximately where they collided).
		/*if (obj2->particle.position_lock) {
			obj1->particle.linearVelocity = glm::vec3(0.f, 0.f, 0.f);
			force -= obj1->particle.gravityAcceleration*25.f;
		}
		else if (obj2->getTag() == "1") {
			obj1->particle.linearVelocity = glm::vec3(0.f,0.f,0.f);
			force -= obj1->particle.gravityAcceleration * 50.f;
		}
		else {*/
			// TODO: Determine if this is even helping
			//glm::vec3 constraint = glm::normalize(glm::vec3{ std::abs(info1.collisionCentroid.x) - std::abs(info2.collisionCentroid.x), std::abs(info1.collisionCentroid.y) - std::abs(info2.collisionCentroid.y), std::abs(info1.collisionCentroid.z) - std::abs(info2.collisionCentroid.z) }) / 150.0f;
			//if (!isnan(constraint.x)) {
			//	force -= constraint;
			//}
			//else {
				force -= glm::normalize(glm::vec3{ std::abs(obj1->transform.translation.x) - std::abs(obj2->transform.translation.x), std::abs(obj1->transform.translation.y) - std::abs(obj2->transform.translation.y), std::abs(obj1->transform.translation.z) - std::abs(obj2->transform.translation.z) }) / 100.0f;
			//}
		//}

		if (obj2->particle.position_lock) {
			obj1->particle.linearVelocity = glm::vec3(0.f, 0.f, 0.f);
			force = obj1->particle.gravityAcceleration * -1.5f;
			//force = glm::vec3(0.f, 0.f, 0.f);
		}
		else if (obj2->getTag() == "1") {
			obj1->particle.linearVelocity = glm::vec3(0.f, 0.f, 0.f);
			force = obj1->particle.gravityAcceleration * -1.5f;
			//force = glm::vec3(0.f, 0.f, 0.f);
		}

		obj1->particle.computeForceAndTorque(force, point);
		// Compute the object's accelerations
		obj1->particle.computeLinearAcceleration();
		if (obj1->getTag() != "1" && obj1->getTag() != "3") {
			obj1->particle.computeAngularAcceleration();
		}
		
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
