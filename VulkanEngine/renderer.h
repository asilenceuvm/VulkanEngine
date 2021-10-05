#pragma once

#include <memory>
#include <vector>

#include "window.h"
#include "pipeline.h"
#include "device.h"
#include "swapchain.h"
#include "model.h"
#include "gameObject.h"

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
	VkDescriptorPool getDescriptorPool() {
		return swapchain->getDescriptorPool();
	}

private:
	Window& window;
	Device& device;
	std::unique_ptr<Swapchain> swapchain;
	std::vector<VkCommandBuffer> commandBuffers;

	uint32_t currentImageIndex;
	int currentFrameIndex{ 0 };
	bool isFrameStarted{ false };

	void createCommandBuffers();
	void freeCommandBuffers();
	void drawFrame();
	void recreateSwapChain();

	void createDescriptorPool();
};

