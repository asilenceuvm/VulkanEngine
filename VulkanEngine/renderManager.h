#pragma once

#include <memory>
#include <vector>

#include "window.h"
#include "pipeline.h"
#include "device.h"
#include "gameObject.h"
#include "camera.h"


class RenderManager {
public:
	RenderManager(Device& device, VkRenderPass renderPass, size_t imageCount);
	~RenderManager();

	//delete copy constructors
	RenderManager(const RenderManager&) = delete;
	RenderManager& operator=(const RenderManager&) = delete;

	void createDescriptorSets(VkDescriptorPool descriptorPool);
	void createUniformBuffers();

	void renderGameObjects(VkCommandBuffer commandBuffer, std::vector<GameObject>& gameObjects, const Camera& camera, int currentFrameIndex);
private:
	Device& device;
	std::unique_ptr<Pipeline> pipeline;
	VkPipelineLayout pipelineLayout;

	size_t imageCount;

	std::vector<VkDescriptorSet> descriptorSets;
	VkDescriptorSetLayout descriptorSetLayout;
	std::vector<VkBuffer> uniformBuffers; //TOOD: rework to use buffer class
	std::vector<VkDeviceMemory> uniformBuffersMemory;

	void createPipelineLayout();
	void createPipeline(VkRenderPass renderPass);
};

