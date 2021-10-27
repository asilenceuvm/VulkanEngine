#pragma once

#include <memory>
#include <array>
#include <vector>

#include "window.h"
#include "pipeline.h"
#include "device.h"
#include "gameObject.h"
#include "camera.h"
#include "texture.h"


class RenderManager {
public:
	RenderManager(Device& device, VkRenderPass renderPass, VkDescriptorSetLayout descriptorSetLayout);
	~RenderManager();

	//delete copy constructors
	RenderManager(const RenderManager&) = delete;
	RenderManager& operator=(const RenderManager&) = delete;

	void renderGameObjects(VkCommandBuffer commandBuffer, std::vector<GameObject>& gameObjects, const Camera& camera, std::vector<VkDeviceMemory> uniformBufferMemory, std::vector<VkDescriptorSet> descriptorSets);
private:
	Device& device;
	std::array<std::unique_ptr<Pipeline>, 4> pipelines;
	VkPipelineLayout pipelineLayout;

	void createPipelineLayout(VkDescriptorSetLayout descriptorSetLayout);
	void createPipeline(VkRenderPass renderPass);
};

