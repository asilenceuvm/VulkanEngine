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

struct PushConstantData {
	alignas(16) glm::mat4 model{ 1.f };
	alignas(16) glm::mat4 view{ 1.f };
	alignas(16) glm::mat4 proj{ 1.f };
	alignas(16) glm::vec3 color;
};

struct UniformBufferObject {
	alignas(16) glm::mat4 model;
	alignas(16) glm::mat4 view;
	alignas(16) glm::mat4 proj;
	alignas(16) glm::vec3 color;
};

RenderManager::RenderManager(Device& device, VkRenderPass renderPass, size_t imageCount) : device{ device }, imageCount{ imageCount } {
	createPipelineLayout();
	createPipeline(renderPass);
}

RenderManager::~RenderManager() {
	//Texture::cleanupTexture(device);
	for (size_t i = 0; i < imageCount; i++) {
        vkDestroyBuffer(device.device(), uniformBuffers[i], nullptr);
        vkFreeMemory(device.device(), uniformBuffersMemory[i], nullptr);
    }
	vkDestroyDescriptorSetLayout(device.device(), descriptorSetLayout, nullptr);
	vkDestroyPipelineLayout(device.device(), pipelineLayout, nullptr);
}


void RenderManager::createPipelineLayout() {
	VkDescriptorSetLayoutBinding uboLayoutBinding{};
    uboLayoutBinding.binding = 0;
    uboLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    uboLayoutBinding.descriptorCount = 1;
	uboLayoutBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;

	VkDescriptorSetLayoutBinding samplerLayoutBinding{};
	samplerLayoutBinding.binding = 1;
	samplerLayoutBinding.descriptorCount = 1;
	samplerLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	samplerLayoutBinding.pImmutableSamplers = nullptr;
	samplerLayoutBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

	std::array<VkDescriptorSetLayoutBinding, 2> bindings = {uboLayoutBinding, samplerLayoutBinding};
	VkDescriptorSetLayoutCreateInfo layoutInfo{};
	layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
	layoutInfo.bindingCount = static_cast<uint32_t>(bindings.size());
	layoutInfo.pBindings = bindings.data();

	if (vkCreateDescriptorSetLayout(device.device(), &layoutInfo, nullptr, &descriptorSetLayout) != VK_SUCCESS) {
		spdlog::critical("Failed to create descriptor set layout");
	}

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

void RenderManager::createUniformBuffers() {
	VkDeviceSize bufferSize = sizeof(UniformBufferObject);

	uniformBuffers.resize(imageCount);
    uniformBuffersMemory.resize(imageCount);

    for (size_t i = 0; i < imageCount; i++) {
        device.createBuffer(bufferSize, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, uniformBuffers[i], uniformBuffersMemory[i]);
    }
}

void RenderManager::createDescriptorSets(VkDescriptorPool descriptorPool) {
	texture = std::make_unique<Texture>(device, "textures/camel.jpg"); //TODO: rework way textures get loaded
	std::vector<VkDescriptorSetLayout> layouts(imageCount, descriptorSetLayout);
	VkDescriptorSetAllocateInfo allocInfo{};
	allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
	allocInfo.descriptorPool = descriptorPool;
	allocInfo.descriptorSetCount = static_cast<uint32_t>(imageCount);
	allocInfo.pSetLayouts = layouts.data();

	descriptorSets.resize(imageCount);
	if (vkAllocateDescriptorSets(device.device(), &allocInfo, descriptorSets.data()) != VK_SUCCESS) {
		throw std::runtime_error("Failed to allocate descriptor sets");
	}

	for (size_t i = 0; i < imageCount; i++) {
		VkDescriptorBufferInfo bufferInfo{};
		bufferInfo.buffer = uniformBuffers[i];
		bufferInfo.offset = 0;
		bufferInfo.range = sizeof(UniformBufferObject);

		VkDescriptorImageInfo imageInfo{};
		imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		//imageInfo.imageView = Texture::createTextureImageView(device);
		imageInfo.sampler = Texture::createTextureSampler(device);
		imageInfo.imageView = texture->createTextureImageView(device);

		std::array<VkWriteDescriptorSet, 2> descriptorWrites{};

		descriptorWrites[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		descriptorWrites[0].dstSet = descriptorSets[i];
		descriptorWrites[0].dstBinding = 0;
		descriptorWrites[0].dstArrayElement = 0;
		descriptorWrites[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		descriptorWrites[0].descriptorCount = 1;
		descriptorWrites[0].pBufferInfo = &bufferInfo;

		descriptorWrites[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		descriptorWrites[1].dstSet = descriptorSets[i];
		descriptorWrites[1].dstBinding = 1;
		descriptorWrites[1].dstArrayElement = 0;
		descriptorWrites[1].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		descriptorWrites[1].descriptorCount = 1;
		descriptorWrites[1].pImageInfo = &imageInfo;

		vkUpdateDescriptorSets(device.device(), static_cast<uint32_t>(descriptorWrites.size()), descriptorWrites.data(), 0, nullptr);
	}
}

void RenderManager::renderGameObjects(VkCommandBuffer commandBuffer, std::vector<GameObject>& gameObjects, const Camera& camera, int currentFrameIndex) {
	pipeline->bind(commandBuffer);

	for (auto& obj : gameObjects) {
		//PushConstantData push{};
		//push.color = obj.color;
		//push.model = obj.transform.mat4();
		//push.view = camera.getView();
		//push.proj = camera.getProjection();
		UniformBufferObject ubo{};
		ubo.color = obj.color;
		ubo.model = obj.transform.mat4();
		ubo.view = camera.getView();
		ubo.proj = camera.getProjection();

		void* data;
		vkMapMemory(device.device(), uniformBuffersMemory[currentFrameIndex], 0, sizeof(ubo), 0, &data);
			memcpy(data, &ubo, sizeof(ubo));
		vkUnmapMemory(device.device(), uniformBuffersMemory[currentFrameIndex]);

		//vkCmdPushConstants(
		//	commandBuffer,
		//	pipelineLayout,
		//	VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
		//	0,
		//	sizeof(PushConstantData),
		//	&push);
		vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, 0, 1, &descriptorSets[currentFrameIndex], 0, nullptr);

		obj.model->bind(commandBuffer);
		obj.model->draw(commandBuffer);
	}
}

