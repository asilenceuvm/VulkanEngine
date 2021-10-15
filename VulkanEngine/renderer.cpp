#include "renderer.h"

#include <stdexcept>
#include <iostream>
#include <thread>

#include "spdlog/spdlog.h"

#include "assetManager.h"
#include "constants.h"
#include "engine.h"


Renderer::Renderer(Window& window, Device& device) : window(window), device(device) {
	AssetManager::loadTexture(device, "models/backpack/diffuse.jpg", true);
	AssetManager::loadTexture(device, "textures/apple.jpg");
	createDescriptorSetLayout();
	recreateSwapChain();
	createCommandBuffers();
}

Renderer::~Renderer() {
	AssetManager::clearTextures();
	vkDestroySampler(device.device(), textureSampler, nullptr);
	vkDestroyDescriptorSetLayout(device.device(), descriptorSetLayout, nullptr);
	for (size_t i = 0; i < 2; i++) {
        vkDestroyBuffer(device.device(), uniformBuffers[i], nullptr);
        vkFreeMemory(device.device(), uniformBuffersMemory[i], nullptr);
    }
	vkDestroyDescriptorPool(device.device(), descriptorPool, nullptr);
	freeCommandBuffers();
}

VkCommandBuffer Renderer::beginFrame() {
	auto result = swapchain->acquireNextImage(&currentImageIndex);

	if (result == VK_ERROR_OUT_OF_DATE_KHR) {
		recreateSwapChain();
		return nullptr;
	}
	if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR) {
		throw std::runtime_error("failed to acquire swap chain image!");
	}
	
	isFrameStarted = true;
	auto commandBuffer = getCurrentCommandBuffer();

	VkCommandBufferBeginInfo beginInfo{};
	beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;

	if (vkBeginCommandBuffer(commandBuffer, &beginInfo) != VK_SUCCESS) {
		throw std::runtime_error("failed to begin recording command buffer!");
	}

	return commandBuffer;

}
void Renderer::endFrame() {
	auto commandBuffer = getCurrentCommandBuffer();
	if (vkEndCommandBuffer(commandBuffer) != VK_SUCCESS) {
		throw std::runtime_error("failed to record command buffer!");
	}

	auto result = swapchain->submitCommandBuffers(&commandBuffer, &currentImageIndex);
	if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR || window.windowResized()) {
		window.resetWindowResizedFlag();
		recreateSwapChain();
	}
	else if (result != VK_SUCCESS) {
		throw std::runtime_error("failed to present swap chain image!");
	}

	isFrameStarted = false;
	currentFrameIndex = (currentFrameIndex + 1) % Swapchain::MAX_FRAMES_IN_FLIGHT;
}
void Renderer::beginSwapChainRenderPass(VkCommandBuffer commandBuffer) {
	VkRenderPassBeginInfo renderPassInfo{};
	renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
	renderPassInfo.renderPass = swapchain->getRenderPass();
	renderPassInfo.framebuffer = swapchain->getFrameBuffer(currentImageIndex);

	renderPassInfo.renderArea.offset = { 0, 0 };
	renderPassInfo.renderArea.extent = swapchain->getSwapChainExtent();

	std::array<VkClearValue, 2> clearValues{};
	clearValues[0].color = { 0.01f, 0.01f, 0.01f, 1.0f };
	clearValues[1].depthStencil = { 1.0f, 0 };
	renderPassInfo.clearValueCount = static_cast<uint32_t>(clearValues.size());
	renderPassInfo.pClearValues = clearValues.data();

	vkCmdBeginRenderPass(commandBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

	VkViewport viewport{};
	viewport.x = 0.0f;
	viewport.y = 0.0f;
	viewport.width = static_cast<float>(swapchain->getSwapChainExtent().width);
	viewport.height = static_cast<float>(swapchain->getSwapChainExtent().height);
	viewport.minDepth = 0.0f;
	viewport.maxDepth = 1.0f;
	VkRect2D scissor{{0, 0}, swapchain->getSwapChainExtent()};
	vkCmdSetViewport(commandBuffer, 0, 1, &viewport);
	vkCmdSetScissor(commandBuffer, 0, 1, &scissor);


}
void Renderer::endSwapChainRenderPass(VkCommandBuffer commandBuffer) {
	vkCmdEndRenderPass(commandBuffer);
}



void Renderer::createCommandBuffers() {
	commandBuffers.resize(Swapchain::MAX_FRAMES_IN_FLIGHT);

	VkCommandBufferAllocateInfo allocInfo{};
	allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	allocInfo.commandPool = device.getCommandPool();
	allocInfo.commandBufferCount = static_cast<uint32_t>(commandBuffers.size());

	if (vkAllocateCommandBuffers(device.device(), &allocInfo, commandBuffers.data()) !=
		VK_SUCCESS) {
		throw std::runtime_error("failed to allocate command buffers!");
	}
}

void Renderer::freeCommandBuffers() {
	vkFreeCommandBuffers(device.device(), device.getCommandPool(), static_cast<float>(commandBuffers.size()), commandBuffers.data());
	commandBuffers.clear();
}


void Renderer::recreateSwapChain() {
	auto extent = window.getExtent();
	while (extent.width == 0 || extent.height == 0) {
		extent = window.getExtent();
		glfwWaitEvents();
	}
	vkDeviceWaitIdle(device.device());

	if (swapchain == nullptr) {
		swapchain = std::make_unique<Swapchain>(device, extent);
	} else {
		std::shared_ptr<Swapchain> oldSwapchain = std::move(swapchain);
		swapchain = std::make_unique<Swapchain>(device, extent, oldSwapchain);

		if (!oldSwapchain->comapreSwapFormats(*swapchain.get())) {
			spdlog::critical("Swap chain image or depth changed");
		}
	}
	createUniformBuffers();
	createDescriptorPool();
	createDescriptorSets();

}

void Renderer::createUniformBuffers() {
	VkDeviceSize bufferSize = sizeof(Constants::UniformBufferObject);

	uniformBuffers.resize(2);
    uniformBuffersMemory.resize(2);

    for (size_t i = 0; i < 2; i++) {
        device.createBuffer(bufferSize, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, uniformBuffers[i], uniformBuffersMemory[i]);
    }
}

void Renderer::createTextureSampler() {
	VkSamplerCreateInfo samplerInfo{};

	samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
	samplerInfo.magFilter = VK_FILTER_LINEAR;
	samplerInfo.minFilter = VK_FILTER_LINEAR;
	samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
	samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
	samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
	samplerInfo.anisotropyEnable = VK_TRUE;
	samplerInfo.maxAnisotropy = device.properties.limits.maxSamplerAnisotropy;
	samplerInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
	samplerInfo.unnormalizedCoordinates = VK_FALSE;
	samplerInfo.compareEnable = VK_FALSE;
	samplerInfo.compareOp = VK_COMPARE_OP_ALWAYS;
	samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;

	if (vkCreateSampler(device.device(), &samplerInfo, nullptr, &textureSampler) != VK_SUCCESS) {
		spdlog::critical("Failed to create texture sampler");
	}
}

void Renderer::createDescriptorSets() {
	createTextureSampler();
	std::vector<VkDescriptorSetLayout> layouts(2, descriptorSetLayout);
	VkDescriptorSetAllocateInfo allocInfo{};
	allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
	allocInfo.descriptorPool = descriptorPool;
	allocInfo.descriptorSetCount = static_cast<uint32_t>(2);
	allocInfo.pSetLayouts = layouts.data();

	descriptorSets.resize(2);
	if (vkAllocateDescriptorSets(device.device(), &allocInfo, descriptorSets.data()) != VK_SUCCESS) {
		throw std::runtime_error("Failed to allocate descriptor sets");
	}

	for (size_t i = 0; i < 2; i++) {
		updateDescriptorSets(i);
	}
}

//int i is the descriptor set to update
void Renderer::updateDescriptorSets(int i) {
	VkDescriptorBufferInfo bufferInfo{};
	bufferInfo.buffer = uniformBuffers[i];
	bufferInfo.offset = 0;
	bufferInfo.range = sizeof(Constants::UniformBufferObject);

	VkDescriptorImageInfo imageInfo{};
	imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
	imageInfo.sampler = textureSampler; 
	imageInfo.imageView = AssetManager::textures[i]->getImageView();

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

void Renderer::createDescriptorPool() {
	std::array<VkDescriptorPoolSize, 2> poolSizes{};
	poolSizes[0].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	poolSizes[0].descriptorCount = static_cast<uint32_t>(swapchain->imageCount());
	poolSizes[1].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	poolSizes[1].descriptorCount = static_cast<uint32_t>(swapchain->imageCount());

	VkDescriptorPoolCreateInfo poolInfo{};
	poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
	poolInfo.poolSizeCount = static_cast<uint32_t>(poolSizes.size());
	poolInfo.pPoolSizes = poolSizes.data();
	poolInfo.maxSets = static_cast<uint32_t>(swapchain->imageCount());

	if (vkCreateDescriptorPool(device.device(), &poolInfo, nullptr, &descriptorPool) != VK_SUCCESS) {
		spdlog::critical("Failed to create descriptor pool");
	}
}

void Renderer::createDescriptorSetLayout() {
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

}
