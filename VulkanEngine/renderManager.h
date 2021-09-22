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
	RenderManager(Device& device, VkRenderPass renderPass);
	~RenderManager();

	//delete copy constructors
	RenderManager(const RenderManager&) = delete;
	RenderManager& operator=(const RenderManager&) = delete;

	void renderGameObjects(VkCommandBuffer commandBuffer, std::vector<GameObject>& gameObjects, const Camera& camera);
private:
	Device& device;
	std::unique_ptr<Pipeline> pipeline;
	VkPipelineLayout pipelineLayout;

	void createPipelineLayout();
	void createPipeline(VkRenderPass renderPass);
};

