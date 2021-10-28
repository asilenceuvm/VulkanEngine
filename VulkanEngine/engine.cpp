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

std::unique_ptr<Model> Engine::generateMesh(int length, int width, std::shared_ptr<Texture> texture, std::string heightmap) {
	Model::Geometry geometry;

	int total = 0;
	//generate length * width squares
	for (int i = 0; i < length; i++) {
		for (int j = 0; j < width; j++) {
			float yPos[4] = { 0.0f, 0.0f, 0.0f, 0.0f };
			Model::Vertex vert1{ {i - .5f, yPos[0], j - .5f}, {1.f, 1.f, 1.f}, {.0f, 1.f, 0.0f}, {1.f, 0.f} };
			Model::Vertex vert2{ {i - .5f, yPos[1], j + .5f}, {1.f, 1.f, 1.f}, {.0f, 1.f, 0.0f}, {0.f, 0.f} };
			Model::Vertex vert3{ {i + .5f, yPos[2], j + .5f}, {1.f, 1.f, 1.f}, {.0f, 1.f, 0.0f}, {0.f, 1.f} };
			Model::Vertex vert4{ {i + .5f, yPos[3], j - .5f}, {1.f, 1.f, 1.f}, {.0f, 1.f, 0.0f}, {1.f, 1.f} };

			geometry.vertices.push_back(vert1);
			geometry.vertices.push_back(vert2);
			geometry.vertices.push_back(vert3);
			geometry.vertices.push_back(vert4);

			//0, 1, 2, 2, 3, 0
			geometry.indices.push_back((total) * 4);
			geometry.indices.push_back(1 + (total) * 4);
			geometry.indices.push_back(2 + (total) * 4);
			geometry.indices.push_back(2 + (total) * 4);
			geometry.indices.push_back(3 + (total) * 4);
			geometry.indices.push_back((total) * 4);
			total++;
		}
	}

	return std::make_unique<Model>(device, geometry, texture);
}

/*template <class T>
void wrapArrayInVector( T *sourceArray, size_t arraySize, std::vector<T, std::allocator<T> > &targetVector ) {
  typename std::_Vector_base<T, std::allocator<T> >::_Vector_impl *vectorPtr =
    (typename std::_Vector_base<T, std::allocator<T> >::_Vector_impl *)((void *) &targetVector);
  vectorPtr->_M_start = sourceArray;
  vectorPtr->_M_finish = vectorPtr->_M_end_of_storage = vectorPtr->_M_start + arraySize;
}*/

// Encapsulate height map data for easy sampling
struct HeightMap
{
private:
	uint16_t *heightdata;
	uint32_t dim;
	uint32_t scale;
public:
	HeightMap(Device& device, std::string filename, uint32_t patchsize) {
		int texWidth, texHeight, texChannels;
		stbi_uc* pixels = stbi_load(filename.c_str(), &texWidth, &texHeight, &texChannels, STBI_rgb_alpha);
		VkDeviceSize imageSize = texWidth * texHeight * 4;

		if (!pixels) {
			spdlog::critical("Failed to load texture image");
		}

		VkBuffer stagingBuffer;
		VkDeviceMemory stagingBufferMemory;
		device.createBuffer(imageSize, 
			VK_BUFFER_USAGE_TRANSFER_SRC_BIT, 
			VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, 
			stagingBuffer, 
			stagingBufferMemory);

		void* data;
		vkMapMemory(device.device(), stagingBufferMemory, 0, imageSize, 0, &data);
			memcpy(data, pixels, static_cast<size_t>(imageSize));
		vkUnmapMemory(device.device(), stagingBufferMemory);

		dim = texWidth;
		heightdata = new uint16_t[dim * dim];
		//this->scale = dim / patchsize;
		this->scale = 2;

		stbi_image_free(pixels);
	};

	~HeightMap() {
		delete[] heightdata;
	}

	float getHeight(uint32_t x, uint32_t y)
	{
		glm::ivec2 rpos = glm::ivec2(x, y) * glm::ivec2(scale);
		rpos.x = std::max(0, std::min(rpos.x, (int)dim-1));
		rpos.y = std::max(0, std::min(rpos.y, (int)dim-1));
		rpos /= glm::ivec2(scale);
		return *(heightdata + (rpos.x + rpos.y * dim) * scale) / 65535.0f;
	}
};

// Generate a terrain quad patch for feeding to the tessellation control shader
std::unique_ptr<Model>generateTerrain(Device& device) {
	#define PATCH_SIZE 64
	#define UV_SCALE 1.0f

	const uint32_t vertexCount = PATCH_SIZE * PATCH_SIZE;
	// We use the Vertex definition from the glTF model loader, so we can re-use the vertex input state
	Model::Vertex* vertices = new Model::Vertex[vertexCount];

	const float wx = 2.0f;
	const float wy = 2.0f;

	for (auto x = 0; x < PATCH_SIZE; x++)
	{
		for (auto y = 0; y < PATCH_SIZE; y++)
		{
			uint32_t index = (x + y * PATCH_SIZE);
			vertices[index].position[0] = x * wx + wx / 2.0f - (float)PATCH_SIZE * wx / 2.0f;
			vertices[index].position[1] = 0.0f;
			vertices[index].position[2] = y * wy + wy / 2.0f - (float)PATCH_SIZE * wy / 2.0f;
			vertices[index].texCoord = glm::vec2((float)x / PATCH_SIZE, (float)y / PATCH_SIZE) * UV_SCALE;
			vertices[index].color = glm::vec3(1);
			vertices[index].normal = glm::vec3(1);
		}
	}

	// Calculate normals from height map using a sobel filter
	HeightMap heightMap(device, "textures/heightmap.png", PATCH_SIZE);
	for (auto x = 0; x < PATCH_SIZE; x++) {
		for (auto y = 0; y < PATCH_SIZE; y++) {
			// Get height samples centered around current position
			float heights[3][3];
			for (auto hx = -1; hx <= 1; hx++)
			{
				for (auto hy = -1; hy <= 1; hy++)
				{
					heights[hx + 1][hy + 1] = heightMap.getHeight(x + hx, y + hy);
				}
			}

			// Calculate the normal
			glm::vec3 normal;
			// Gx sobel filter
			normal.x = heights[0][0] - heights[2][0] + 2.0f * heights[0][1] - 2.0f * heights[2][1] + heights[0][2] - heights[2][2];
			// Gy sobel filter
			normal.z = heights[0][0] + 2.0f * heights[1][0] + heights[2][0] - heights[0][2] - 2.0f * heights[1][2] - heights[2][2];
			// Calculate missing up component of the normal using the filtered x and y axis
			// The first value controls the bump strength
			normal.y = 0.25f * sqrt(1.0f - normal.x * normal.x - normal.z * normal.z);

			vertices[x + y * PATCH_SIZE].normal = glm::normalize(normal * glm::vec3(2.0f, 1.0f, 2.0f));
		}
	}

	// Indices
	const uint32_t w = (PATCH_SIZE - 1);
	const uint32_t indexCount = w * w * 4;
	uint32_t* indices = new uint32_t[indexCount];
	for (auto x = 0; x < w; x++)
	{
		for (auto y = 0; y < w; y++)
		{
			uint32_t index = (x + y * w) * 4;
			indices[index] = (x + y * PATCH_SIZE);
			indices[index + 1] = indices[index] + PATCH_SIZE;
			indices[index + 2] = indices[index + 1] + 1;
			indices[index + 3] = indices[index] + 1;
		}
	}
	Model::Geometry geometry;
	geometry.vertices = std::vector<Model::Vertex>(vertices, vertices + (PATCH_SIZE * PATCH_SIZE));
	geometry.indices = std::vector<uint32_t>(indices, indices + (PATCH_SIZE * PATCH_SIZE));

	return std::make_unique<Model>(device, geometry, AssetManager::textures["sand"]);
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
	gameObj1.model = generateMesh(100, 100, AssetManager::textures["skybox"]);
	gameObj1.transform.scale = glm::vec3(0.05f, 0.05f, 0.05f);
	gameObjects.push_back(std::move(gameObj1));

	auto gameObj4 = GameObject::createGameObject("terrain");
	gameObj4.model = //generateMesh(25, 25, AssetManager::textures["sand"], "textures/heightmap.png");
	gameObj4.model = generateTerrain(device);
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


	lightPos = glm::vec3(0, 1, 3);

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

//eventually should handle all shutdown procedures
void Engine::shutdown() {
	window.setWindowShouldClose();
}
