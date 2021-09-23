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


struct PushConstantData {
	alignas(16) glm::mat4 model{ 1.f };
	alignas(16) glm::mat4 view{ 1.f };
	alignas(16) glm::mat4 proj{ 1.f };
	alignas(16) glm::vec3 color;
};

RenderManager::RenderManager(Device& device, VkRenderPass renderPass) : device{ device } {
	createPipelineLayout();
	createPipeline(renderPass);
}

RenderManager::~RenderManager() {
	vkDestroyPipelineLayout(device.device(), pipelineLayout, nullptr);
}


void RenderManager::createPipelineLayout() {
	VkPushConstantRange pushConstantRange{};
	pushConstantRange.stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
	pushConstantRange.offset = 0;
	pushConstantRange.size = sizeof(PushConstantData);

	VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
	pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
	pipelineLayoutInfo.setLayoutCount = 0;
	pipelineLayoutInfo.pSetLayouts = nullptr;
	pipelineLayoutInfo.pushConstantRangeCount = 1;
	pipelineLayoutInfo.pPushConstantRanges = &pushConstantRange;

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


void RenderManager::renderGameObjects(VkCommandBuffer commandBuffer, std::vector<GameObject>& gameObjects, const Camera& camera) {
	pipeline->bind(commandBuffer);

	for (auto& obj : gameObjects) {
		PushConstantData push{};
		push.color = obj.color;
		push.model = obj.transform.mat4();
		push.view = camera.getView();
		push.proj = camera.getProjection();


		vkCmdPushConstants(
			commandBuffer,
			pipelineLayout,
			VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
			0,
			sizeof(PushConstantData),
			&push);

		obj.model->bind(commandBuffer);
		obj.model->draw(commandBuffer);
	}
}

