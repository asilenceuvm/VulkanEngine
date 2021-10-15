#pragma once

#include <memory>
#include <vector>

#include "window.h"
#include "pipeline.h"
#include "device.h"
#include "swapchain.h"
#include "model.h"
#include "gameObject.h"
#include "texture.h"

class Renderer {
public:
	Renderer(Window& window, Device& device);
	~Renderer();

	//delete copy constructors
	Renderer(const Renderer&) = delete;
	Renderer& operator=(const Renderer&) = delete;

	VkCommandBuffer beginFrame();
	void endFrame();
	void beginSwapChainRenderPass(VkCommandBuffer commandBuffer);
	void endSwapChainRenderPass(VkCommandBuffer commandBuffer);

	void createUniformBuffers();
	void createDescriptorSets();
	void updateDescriptorSets(int i);

	//getters
	VkRenderPass getSwapChainRenderPass() const { return swapchain->getRenderPass(); }
	bool isFrameInProgress() const { return isFrameStarted; }
	VkCommandBuffer getCurrentCommandBuffer() const {
		return commandBuffers[currentFrameIndex];
	}
	int getFrameIndex() const {
		return currentFrameIndex;
	}
	float getAspectRatio() const {
		return swapchain->extentAspectRatio();
	}
	size_t getImageCount() const {
		return swapchain->imageCount();
	}
	std::vector<VkDeviceMemory> getUniformBuffersMemory() {
		return uniformBuffersMemory;
	}
	std::vector<VkBuffer> getUniformBuffers() {
		return uniformBuffers;
	}
	std::vector<VkDescriptorSet> getDescriptorSets() {
		return descriptorSets;
	}
	VkDescriptorSetLayout getDescriptorSetLayout() {
		return descriptorSetLayout;
	}

private:
	Window& window;
	Device& device;
	std::unique_ptr<Swapchain> swapchain;
	std::vector<VkCommandBuffer> commandBuffers;

	uint32_t currentImageIndex;
	int currentFrameIndex{ 0 };
	bool isFrameStarted{ false };

	VkDescriptorPool descriptorPool;
	std::vector<VkDescriptorSet> descriptorSets;
	VkDescriptorSetLayout descriptorSetLayout;

	std::vector<VkBuffer> uniformBuffers; //TOOD: rework to use buffer class
	std::vector<VkDeviceMemory> uniformBuffersMemory;

	//std::unique_ptr<Texture> texture;
	Texture tex{ device, "textures/camel.jpg" };
	VkSampler textureSampler;

	void createCommandBuffers();
	void freeCommandBuffers();
	void recreateSwapChain();
	void createTextureSampler();
	void createDescriptorPool();
	void createDescriptorSetLayout();
};

