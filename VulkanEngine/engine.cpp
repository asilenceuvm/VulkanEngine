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
#include "stb_image_write.h"

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
std::vector<std::vector<char*>> Engine::curImage;
std::atomic<bool> Engine::takeImage = false;

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

void Engine::addModel() {
	AssetManager::loadTexture(device, modelTexture, modelTexture);
	AssetManager::loadModel(device, modelToAdd, modelFilepath, modelTexture); 
	modelToAdd = "";
	modelFilepath = "";
	modelTexture = "";
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

void Engine::getCurrentImage() {
	bool supportsBlit = device.getBlitSupport();
	VkImage srcImage = renderer.getCurrentImage();

	VkImageCreateInfo imageCreateCI{};
	imageCreateCI.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
	imageCreateCI.imageType = VK_IMAGE_TYPE_2D;
	imageCreateCI.format = VK_FORMAT_R8G8B8A8_SRGB;
	imageCreateCI.extent.width = width;
	imageCreateCI.extent.height = height;
	imageCreateCI.extent.depth = 1;
	imageCreateCI.arrayLayers = 1;
	imageCreateCI.mipLevels = 1;
	imageCreateCI.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	imageCreateCI.samples = VK_SAMPLE_COUNT_1_BIT;
	imageCreateCI.tiling = VK_IMAGE_TILING_LINEAR;
	imageCreateCI.usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT;

	VkImage dstImage;
	vkCreateImage(device.device(), &imageCreateCI, nullptr, &dstImage);
	VkMemoryRequirements memRequirements;
	VkMemoryAllocateInfo memAllocInfo{};
	memAllocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	VkDeviceMemory dstImageMemory;
	vkGetImageMemoryRequirements(device.device(), dstImage, &memRequirements);
	memAllocInfo.allocationSize = memRequirements.size;

	memAllocInfo.memoryTypeIndex = device.findMemoryType(memRequirements.memoryTypeBits, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
	vkAllocateMemory(device.device(), &memAllocInfo, nullptr, &dstImageMemory);
	vkBindImageMemory(device.device(), dstImage, dstImageMemory, 0);

	VkCommandBufferAllocateInfo commandBufferAllocateInfo {};
	commandBufferAllocateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	commandBufferAllocateInfo.commandPool = device.getCommandPool();
	commandBufferAllocateInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	commandBufferAllocateInfo.commandBufferCount = 1;
	VkCommandBuffer copyCmd;
	vkAllocateCommandBuffers(device.device(), &commandBufferAllocateInfo, &copyCmd);
	VkCommandBufferBeginInfo cmdBufInfo{};
	cmdBufInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	vkBeginCommandBuffer(copyCmd, &cmdBufInfo);

	Utils::insertImageMemoryBarrier(
		copyCmd,
		dstImage,
		0,
		VK_ACCESS_TRANSFER_WRITE_BIT,
		VK_IMAGE_LAYOUT_UNDEFINED,
		VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
		VK_PIPELINE_STAGE_TRANSFER_BIT,
		VK_PIPELINE_STAGE_TRANSFER_BIT,
		VkImageSubresourceRange{ VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 });

	Utils::insertImageMemoryBarrier(
		copyCmd,
		srcImage,
		VK_ACCESS_MEMORY_READ_BIT,
		VK_ACCESS_TRANSFER_READ_BIT,
		VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
		VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
		VK_PIPELINE_STAGE_TRANSFER_BIT,
		VK_PIPELINE_STAGE_TRANSFER_BIT,
		VkImageSubresourceRange{ VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 });

	if (supportsBlit) {
		VkOffset3D blitSize;
		blitSize.x = width;
		blitSize.y = height;
		blitSize.z = 1;
		VkImageBlit imageBlitRegion{};
		imageBlitRegion.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		imageBlitRegion.srcSubresource.layerCount = 1;
		imageBlitRegion.srcOffsets[1] = blitSize;
		imageBlitRegion.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		imageBlitRegion.dstSubresource.layerCount = 1;
		imageBlitRegion.dstOffsets[1] = blitSize;

		vkCmdBlitImage(
			copyCmd,
			srcImage, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
			dstImage, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
			1,
			&imageBlitRegion,
			VK_FILTER_NEAREST);
	}
	else {
		VkImageCopy imageCopyRegion{};
		imageCopyRegion.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		imageCopyRegion.srcSubresource.layerCount = 1;
		imageCopyRegion.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		imageCopyRegion.dstSubresource.layerCount = 1;
		imageCopyRegion.extent.width = width;
		imageCopyRegion.extent.height = height;
		imageCopyRegion.extent.depth = 1;

		vkCmdCopyImage(
			copyCmd,
			srcImage, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
			dstImage, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
			1,
			&imageCopyRegion);
	}

	Utils::insertImageMemoryBarrier(
		copyCmd,
		dstImage,
		VK_ACCESS_TRANSFER_WRITE_BIT,
		VK_ACCESS_MEMORY_READ_BIT,
		VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
		VK_IMAGE_LAYOUT_GENERAL,
		VK_PIPELINE_STAGE_TRANSFER_BIT,
		VK_PIPELINE_STAGE_TRANSFER_BIT,
		VkImageSubresourceRange{ VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 });

	Utils::insertImageMemoryBarrier(
		copyCmd,
		srcImage,
		VK_ACCESS_TRANSFER_READ_BIT,
		VK_ACCESS_MEMORY_READ_BIT,
		VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
		VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
		VK_PIPELINE_STAGE_TRANSFER_BIT,
		VK_PIPELINE_STAGE_TRANSFER_BIT,
		VkImageSubresourceRange{ VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 });

	vkEndCommandBuffer(copyCmd);
	VkSubmitInfo submitInfo{};
	submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	submitInfo.commandBufferCount = 1;
	submitInfo.pCommandBuffers = &copyCmd;

	VkFenceCreateInfo fenceCreateInfo {};
	fenceCreateInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
	fenceCreateInfo.flags = 0;

	VkFence fence;
	vkCreateFence(device.device(), &fenceCreateInfo, nullptr, &fence);
	vkQueueSubmit(device.graphicsQueue(), 1, &submitInfo, fence);
	vkWaitForFences(device.device(), 1, &fence, VK_TRUE, 10000000000);
	vkDestroyFence(device.device(), fence, nullptr);
	vkFreeCommandBuffers(device.device(), device.getCommandPool(), 1, &copyCmd);

	VkImageSubresource subResource { VK_IMAGE_ASPECT_COLOR_BIT, 0, 0 };
	VkSubresourceLayout subResourceLayout;
	vkGetImageSubresourceLayout(device.device(), dstImage, &subResource, &subResourceLayout);

	char* data;
	vkMapMemory(device.device(), dstImageMemory, 0, VK_WHOLE_SIZE, 0, (void**)&data);
	//data += subResourceLayout.offset;

	bool colorSwizzle = false;
	if (!supportsBlit) {
		std::vector<VkFormat> formatsBGR = { VK_FORMAT_B8G8R8A8_SRGB, VK_FORMAT_B8G8R8A8_UNORM, VK_FORMAT_B8G8R8A8_SNORM };
		colorSwizzle = (std::find(formatsBGR.begin(), formatsBGR.end(), VK_FORMAT_R8G8B8A8_UNORM) != formatsBGR.end());
	}

	stbi_write_jpg("temp.jpg", width, height, 4, data, 100);

	//curImage = data;
	std::vector<std::vector<char*>> image;
	image.resize(3);
	for (uint32_t y = 0; y < height; y++) {
			unsigned int *row = (unsigned int*)data;
			for (uint32_t x = 0; x < width; x++) {
				//if (colorSwizzle) {
					image[0].push_back((char*)row+2);
					image[1].push_back((char*)row+1);
					image[2].push_back((char*)row+0);
				//}
				//else {
					//image.push_back((char*)row+0);
					//file.write((char*)row, 3);
				//}
				row++;
			}
			data += subResourceLayout.rowPitch;
		}
	curImage.assign(image.begin(), image.end());
	//curImage = image;
	//spdlog::debug("{}", image);
	
	vkUnmapMemory(device.device(), dstImageMemory);
	vkFreeMemory(device.device(), dstImageMemory, nullptr);
	vkDestroyImage(device.device(), dstImage, nullptr);
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
	//AssetManager::loadTexture(device, "textures/apple.jpg", "apple");
	AssetManager::loadTexture(device, "textures/sand.jpg", "sand");
	AssetManager::loadTexture(device, "models/rock/rock.tga", "rock");
	std::array<std::string, 6> filepaths = {
		"textures/skybox4/right.png",
		"textures/skybox4/left.png",
		"textures/skybox4/top.png",
		"textures/skybox4/bottom.png",
		"textures/skybox4/front.png",
		"textures/skybox4/back.png"
	};
	AssetManager::loadCubeMap(device, filepaths, "skybox");

	AssetManager::loadModel(device, "backpack", "models/backpack/backpack.obj", "backpack"); 
	AssetManager::loadModel(device, "rock", "models/rock/rock.obj", "rock"); 
	//AssetManager::loadModel(device, "apple", "models/apple.obj", "apple"); 
	AssetManager::loadModel(device, "skybox", "models/textured_cube.obj", "skybox"); 

	auto gameObj = GameObject::createGameObject("backpack");
	gameObj.model = AssetManager::models["backpack"];
	gameObj.transform.translation = { -15.0f, 1.5f, 2.5f };
	gameObj.transform.scale = glm::vec3(0.5f);
	gameObjects.push_back(std::move(gameObj));

	auto gameObj1 = GameObject::createGameObject("water");
	gameObj1.model = Model::generateMesh(device, 1600, 1600, AssetManager::textures["skybox"]);
	gameObj1.transform.scale = glm::vec3(0.1f);
	gameObj1.transform.translation = glm::vec3(-75, 1, -75);
	gameObjects.push_back(std::move(gameObj1));

	auto gameObj4 = GameObject::createGameObject("terrain");
	gameObj4.model = Model::generateTerrain(device, 64, 1.f);
	gameObj4.transform.translation = glm::vec3(0, 2, 0);
	gameObjects.push_back(std::move(gameObj4));

	auto gameObj5 = GameObject::createGameObject("rock");
	gameObj5.model = AssetManager::models["rock"];
	gameObj5.transform.translation = glm::vec3(-10, 0.2, 10);
	gameObj5.transform.rotation = glm::vec3(-10, 0, 0);
	gameObj5.transform.scale = glm::vec3(0.01);
	gameObjects.push_back(std::move(gameObj5));

	/*for (int i = 0; i < 5; i++) {
		auto gameObj2 = GameObject::createGameObject("apple" + std::to_string(i));
		gameObj2.model = AssetManager::models["apple"];
		gameObj2.transform.translation = { 1.f, .0f + i * 0.1f, 2.5f };
		gameObj2.transform.scale = glm::vec3(1.f);
		gameObjects.push_back(std::move(gameObj2));
	}*/


	auto gameObj3 = GameObject::createGameObject("skybox");
	gameObj3.model = AssetManager::models["skybox"];
	gameObjects.push_back(std::move(gameObj3));


	lightPos = glm::vec3(120, 30, 250);

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
	PythonManager::runUpdates();
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
		//std::thread(&Engine::addModel, this).detach();
		addModel();
	}
	if (takeImage) {
		mtx.lock();
		getCurrentImage();
		mtx.unlock();
		takeImage = false;
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
