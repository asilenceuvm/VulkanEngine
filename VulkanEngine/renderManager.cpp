#include "renderManager.h"

#include <stdexcept>
#include <iostream>
#include <thread>

#include "spdlog/spdlog.h"

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include "glm/glm.hpp"
#include "glm/gtc/constants.hpp"
#include "glm/gtx/string_cast.hpp"

#include "buffer.h"
#include "texture.h"
#include "constants.h"
#include "engine.h"
#include "descriptorManager.h"


RenderManager::RenderManager(Device& device, VkRenderPass renderPass) : device{ device } {
	createPipelineLayout();
	createPipeline(renderPass);
}

RenderManager::~RenderManager() {
	vkDestroyPipelineLayout(device.device(), pipelineLayouts.object, nullptr);
	vkDestroyPipelineLayout(device.device(), pipelineLayouts.terrain, nullptr);
}


void RenderManager::createPipelineLayout() {
	VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
	pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
	pipelineLayoutInfo.setLayoutCount = 1;
	pipelineLayoutInfo.pSetLayouts = &DescriptorManager::descriptorSetLayouts.object;
	pipelineLayoutInfo.pushConstantRangeCount = 0;

	if (vkCreatePipelineLayout(device.device(), &pipelineLayoutInfo, nullptr, &pipelineLayouts.object) != VK_SUCCESS) {
		spdlog::critical("Failed to create pipeline layout");
		throw std::runtime_error("createPipelineLayout");
	}

	VkPipelineLayoutCreateInfo pipelineLayoutInfo2{};
	pipelineLayoutInfo2.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
	pipelineLayoutInfo2.setLayoutCount = 1;
	pipelineLayoutInfo2.pSetLayouts = &DescriptorManager::descriptorSetLayouts.terrain;
	pipelineLayoutInfo2.pushConstantRangeCount = 0;

	if (vkCreatePipelineLayout(device.device(), &pipelineLayoutInfo2, nullptr, &pipelineLayouts.terrain) != VK_SUCCESS) {
		spdlog::critical("Failed to create pipeline layout");
		throw std::runtime_error("createPipelineLayout");
	}
}
void RenderManager::createPipeline(VkRenderPass renderPass) {
	//skybox
	PipelineConfigInfo pipelineConfigCube{};
	Pipeline::defaultPipelineConfigInfo(pipelineConfigCube);
	pipelineConfigCube.renderPass = renderPass;
	pipelineConfigCube.pipelineLayout = pipelineLayouts.object;
	pipelineConfigCube.rasterizationInfo.cullMode = VK_CULL_MODE_NONE;
	pipelineConfigCube.depthStencilInfo.depthWriteEnable = VK_FALSE;
	pipelineConfigCube.depthStencilInfo.depthTestEnable = VK_FALSE;
	pipelines[0] = std::make_unique<Pipeline>(device, "shaders/cubemapvert.spv", "shaders/cubemapfrag.spv", pipelineConfigCube);

	//3d objects
	PipelineConfigInfo pipelineConfig{};
	Pipeline::defaultPipelineConfigInfo(pipelineConfig);
	pipelineConfig.renderPass = renderPass;
	pipelineConfig.pipelineLayout = pipelineLayouts.object;
	pipelineConfig.rasterizationInfo.cullMode = VK_CULL_MODE_BACK_BIT;
	pipelineConfig.depthStencilInfo.depthWriteEnable = VK_TRUE;
	pipelineConfig.depthStencilInfo.depthTestEnable = VK_TRUE;
	pipelines[1] = std::make_unique<Pipeline>(device, "shaders/vert.spv", "shaders/frag.spv", pipelineConfig);

	//water
	PipelineConfigInfo pipelineConfigWater{};
	Pipeline::defaultPipelineConfigInfo(pipelineConfigWater);
	pipelineConfigWater.renderPass = renderPass;
	pipelineConfigWater.pipelineLayout = pipelineLayouts.object;
	pipelines[2] = std::make_unique<Pipeline>(device, "shaders/watervert.spv", "shaders/waterfrag.spv", pipelineConfigWater);

	//terrain 
	PipelineConfigInfo pipelineConfigTerrain{};
	Pipeline::defaultPipelineConfigInfo(pipelineConfigTerrain);
	pipelineConfigTerrain.renderPass = renderPass;
	pipelineConfigTerrain.pipelineLayout = pipelineLayouts.terrain;
	pipelines[3] = std::make_unique<Pipeline>(device, "shaders/vert.spv", "shaders/frag.spv", pipelineConfigTerrain, "shaders/tese.spv", "shaders/tesc.spv");
	spdlog::debug("{}", pipelines.size());
}


void RenderManager::renderGameObjects(VkCommandBuffer commandBuffer, 
		std::vector<GameObject>& gameObjects, 
		const Camera& camera, std::vector<VkDeviceMemory> uniformBuffersMemory) {
	//TODO: rework multi pipeline system 

	//cubemap
	pipelines[0]->bind(commandBuffer);
	auto it = std::find_if(std::begin(Engine::gameObjects), std::end(Engine::gameObjects), [&](GameObject const& obj) { return obj.getTag() == "skybox"; });
	auto index = std::distance(Engine::gameObjects.begin(), it);
	Constants::CubeMapUBO ubo{};
	ubo.view = glm::mat4(glm::mat3(camera.getView()));  
	ubo.proj = camera.getProjection();

	void* data;
	vkMapMemory(device.device(), uniformBuffersMemory[index], 0, sizeof(ubo), 0, &data);
		memcpy(data, &ubo, sizeof(ubo));
	vkUnmapMemory(device.device(), uniformBuffersMemory[index]);

	vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayouts.object, 0, 1, &DescriptorManager::descriptorSets.objects[index], 0, nullptr);

	gameObjects.back().model->bind(commandBuffer);
	gameObjects.back().model->draw(commandBuffer);

	
	//game object pipeline
	pipelines[1]->bind(commandBuffer);
	//for (auto& obj : gameObjects) {
	for (int i = 0; i < gameObjects.size(); i++) {
		if (gameObjects[i].getTag() != "water" && 
				gameObjects[i].getTag() != "skybox" &&
				gameObjects[i].getTag() != "terrain") {
			Constants::ObjectUBO ubo{};
			ubo.lightPos = Engine::lightPos;
			ubo.viewPos = camera.getCameraPos();
			ubo.model = gameObjects[i].transform.mat4();
			ubo.view = camera.getView();
			ubo.proj = camera.getProjection();

			void* data;
			vkMapMemory(device.device(), uniformBuffersMemory[gameObjects[i].getId()], 0, sizeof(ubo), 0, &data);
			memcpy(data, &ubo, sizeof(ubo));
			vkUnmapMemory(device.device(), uniformBuffersMemory[gameObjects[i].getId()]);

			vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayouts.object, 0, 1, &DescriptorManager::descriptorSets.objects[gameObjects[i].getId()], 0, nullptr);

			gameObjects[i].model->bind(commandBuffer);
			gameObjects[i].model->draw(commandBuffer);
		}
	}

	//water 
	pipelines[2]->bind(commandBuffer);
	it = std::find_if(std::begin(Engine::gameObjects), std::end(Engine::gameObjects), [&](GameObject const& obj) { return obj.getTag() == "water"; });
	index = std::distance(Engine::gameObjects.begin(), it);
	Constants::ObjectUBO waterubo{};
	waterubo.lightPos = Engine::lightPos;
	waterubo.viewPos = camera.getCameraPos();
	waterubo.model = gameObjects[index].transform.mat4();
	waterubo.view = camera.getView();
	waterubo.proj = camera.getProjection();
	waterubo.time = glfwGetTime();

	void* dataWater;
	vkMapMemory(device.device(), uniformBuffersMemory[gameObjects[index].getId()], 0, sizeof(waterubo), 0, &dataWater);
	memcpy(dataWater, &waterubo, sizeof(waterubo));
	vkUnmapMemory(device.device(), uniformBuffersMemory[gameObjects[index].getId()]);

	vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayouts.object, 0, 1, &DescriptorManager::descriptorSets.objects[gameObjects[index].getId()], 0, nullptr);

	gameObjects[index].model->bind(commandBuffer);
	gameObjects[index].model->draw(commandBuffer);

	//terrain 
	pipelines[3]->bind(commandBuffer);
	it = std::find_if(std::begin(Engine::gameObjects), std::end(Engine::gameObjects), [&](GameObject const& obj) { return obj.getTag() == "terrain"; });
	index = std::distance(Engine::gameObjects.begin(), it);
	Constants::TesselationUBO tesselationUBO{};

	tesselationUBO.projection = camera.getProjection();
	tesselationUBO.modelview = camera.getView() * gameObjects[index].transform.mat4();
	tesselationUBO.lightPos = glm::vec4(Engine::lightPos, 1); 
	tesselationUBO.viewportDim = glm::vec2((float)800, (float)600);

	void* dataTess;
	vkMapMemory(device.device(), uniformBuffersMemory[gameObjects[index].getId()], 0, sizeof(tesselationUBO), 0, &dataTess);
	memcpy(dataTess, &waterubo, sizeof(waterubo));
	vkUnmapMemory(device.device(), uniformBuffersMemory[gameObjects[index].getId()]);

	vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayouts.terrain, 0, 1, &DescriptorManager::descriptorSets.terrain, 0, nullptr);

	gameObjects[index].model->bind(commandBuffer);
	gameObjects[index].model->draw(commandBuffer);
}

