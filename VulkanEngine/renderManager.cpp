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
	PipelineConfigInfo pipelineConfig{};
	Pipeline::defaultPipelineConfigInfo(pipelineConfig);
	pipelineConfig.renderPass = renderPass;
	pipelineConfig.pipelineLayout = pipelineLayout;
	pipeline = std::make_unique<Pipeline>(device, "shaders/vert.spv", "shaders/frag.spv", pipelineConfig);
}


void RenderManager::renderGameObjects(VkCommandBuffer commandBuffer, std::vector<GameObject>& gameObjects, const Camera& camera, std::vector<VkDeviceMemory> uniformBuffersMemory, std::vector<VkDescriptorSet> descriptorSets) {
	pipeline->bind(commandBuffer);

	//for (auto& obj : gameObjects) {
	for (int i = 0; i < gameObjects.size(); i++) {

		Constants::UniformBufferObject ubo{};
		ubo.lightPos = glm::vec3(0, 1, 5); //TODO: make light pos a variable
		ubo.model = gameObjects[i].transform.mat4();
		ubo.view = camera.getView();
		ubo.proj = camera.getProjection();

		void* data;
		vkMapMemory(device.device(), uniformBuffersMemory[i], 0, sizeof(ubo), 0, &data);
			memcpy(data, &ubo, sizeof(ubo));
		vkUnmapMemory(device.device(), uniformBuffersMemory[i]);

		vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, 0, 1, &descriptorSets[i], 0, nullptr);

		gameObjects[i].model->bind(commandBuffer);
		gameObjects[i].model->draw(commandBuffer);
	}
}

