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
	//skybox
	PipelineConfigInfo pipelineConfigCube{};
	Pipeline::defaultPipelineConfigInfo(pipelineConfigCube);
	pipelineConfigCube.renderPass = renderPass;
	pipelineConfigCube.pipelineLayout = pipelineLayout;
	pipelineConfigCube.rasterizationInfo.cullMode = VK_CULL_MODE_NONE;
	pipelineConfigCube.depthStencilInfo.depthWriteEnable = VK_FALSE;
	pipelineConfigCube.depthStencilInfo.depthTestEnable = VK_FALSE;
	pipelines[0] = std::make_unique<Pipeline>(device, "shaders/cubemapvert.spv", "shaders/cubemapfrag.spv", pipelineConfigCube);

	//3d objects
	PipelineConfigInfo pipelineConfig{};
	Pipeline::defaultPipelineConfigInfo(pipelineConfig);
	pipelineConfig.renderPass = renderPass;
	pipelineConfig.pipelineLayout = pipelineLayout;
	pipelineConfig.rasterizationInfo.cullMode = VK_CULL_MODE_BACK_BIT;

	pipelineConfig.depthStencilInfo.depthWriteEnable = VK_TRUE;
	pipelineConfig.depthStencilInfo.depthTestEnable = VK_TRUE;

	pipelines[1] = std::make_unique<Pipeline>(device, "shaders/vert.spv", "shaders/frag.spv", pipelineConfig);

}


void RenderManager::renderGameObjects(VkCommandBuffer commandBuffer, std::vector<GameObject>& gameObjects, const Camera& camera, std::vector<VkDeviceMemory> uniformBuffersMemory, std::vector<VkDescriptorSet> descriptorSets) {
	//TODO: rework multi pipeline system 

	//cubemap
	pipelines[0]->bind(commandBuffer);
	Constants::CubeMapUBO ubo{};
	//ubo.view = camera.getView();
	ubo.view = glm::mat4(glm::mat3(camera.getView()));  
	ubo.proj = camera.getProjection();

	void* data;
	vkMapMemory(device.device(), uniformBuffersMemory[gameObjects.size() - 1], 0, sizeof(ubo), 0, &data);
		memcpy(data, &ubo, sizeof(ubo));
	vkUnmapMemory(device.device(), uniformBuffersMemory[gameObjects.size() - 1]);

	vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, 0, 1, &descriptorSets[gameObjects.size() - 1], 0, nullptr);

	gameObjects.back().model->bind(commandBuffer);
	gameObjects.back().model->draw(commandBuffer);

	
	//game object pipeline
	pipelines[1]->bind(commandBuffer);
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

}

