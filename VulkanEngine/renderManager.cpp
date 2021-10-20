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


RenderManager::RenderManager(Device& device, VkRenderPass renderPass, VkDescriptorSetLayout descriptorSetLayout) : device{ device } {
	createPipelineLayout(descriptorSetLayout);
	createPipeline(renderPass);
}

RenderManager::~RenderManager() {
	vkDestroyPipelineLayout(device.device(), pipelineLayout, nullptr);
}


void RenderManager::createPipelineLayout(VkDescriptorSetLayout descriptorSetLayout) {
	VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
	pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
	pipelineLayoutInfo.setLayoutCount = 1;
	pipelineLayoutInfo.pSetLayouts = &descriptorSetLayout;
	pipelineLayoutInfo.pushConstantRangeCount = 0;

	if (vkCreatePipelineLayout(device.device(), &pipelineLayoutInfo, nullptr, &pipelineLayout) != VK_SUCCESS) {
		spdlog::critical("Failed to create pipeline layout");
		throw std::runtime_error("createPipelineLayout");
	}
}
void RenderManager::createPipeline(VkRenderPass renderPass) {
	//3d objects
	PipelineConfigInfo pipelineConfig{};
	Pipeline::defaultPipelineConfigInfo(pipelineConfig, false);
	pipelineConfig.renderPass = renderPass;
	pipelineConfig.pipelineLayout = pipelineLayout;
	pipelines[0] = std::make_unique<Pipeline>(device, "shaders/vert.spv", "shaders/frag.spv", pipelineConfig);

	//skybox
	PipelineConfigInfo pipelineConfig2{};
	Pipeline::defaultPipelineConfigInfo(pipelineConfig2, true);
	pipelineConfig2.renderPass = renderPass;
	pipelineConfig2.pipelineLayout = pipelineLayout;
	pipelines[1] = std::make_unique<Pipeline>(device, "shaders/cubemapvert.spv", "shaders/cubemapfrag.spv", pipelineConfig2);
}


void RenderManager::renderGameObjects(VkCommandBuffer commandBuffer, std::vector<GameObject>& gameObjects, const Camera& camera, std::vector<VkDeviceMemory> uniformBuffersMemory, std::vector<VkDescriptorSet> descriptorSets) {
	//game object pipeline
	pipelines[0]->bind(commandBuffer);
	//for (auto& obj : gameObjects) {
	for (int i = 0; i < gameObjects.size() - 1; i++) {
		Constants::UniformBufferObject ubo{};
		ubo.lightPos = Engine::lightPos; 
		ubo.viewPos = camera.getCameraPos();
		ubo.model = gameObjects[i].transform.mat4();
		ubo.view = camera.getView();
		ubo.proj = camera.getProjection();

		void* data;
		vkMapMemory(device.device(), uniformBuffersMemory[gameObjects[i].getId()], 0, sizeof(ubo), 0, &data);
			memcpy(data, &ubo, sizeof(ubo));
		vkUnmapMemory(device.device(), uniformBuffersMemory[gameObjects[i].getId()]);

		vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, 0, 1, &descriptorSets[gameObjects[i].getId()], 0, nullptr);

		gameObjects[i].model->bind(commandBuffer);
		gameObjects[i].model->draw(commandBuffer);
	}

	//cubemap
	pipelines[1]->bind(commandBuffer);
	Constants::CubeMapUBO ubo{};
	//ubo.view = camera.getView();
	ubo.view = glm::mat4(glm::mat3(camera.getView()));  
	ubo.proj = camera.getProjection();

	void* data;
	vkMapMemory(device.device(), uniformBuffersMemory[gameObjects.back().getId()], 0, sizeof(ubo), 0, &data);
		memcpy(data, &ubo, sizeof(ubo));
	vkUnmapMemory(device.device(), uniformBuffersMemory[gameObjects.back().getId()]);

	vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, 0, 1, &descriptorSets[gameObjects.back().getId()], 0, nullptr);

	gameObjects.back().model->bind(commandBuffer);
	gameObjects.back().model->draw(commandBuffer);
}

