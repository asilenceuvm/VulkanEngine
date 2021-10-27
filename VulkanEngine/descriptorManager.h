#pragma once

#include "vulkan/vulkan.h"

#include "device.h"
#include "gameObject.h"

class DescriptorManager {
public:
	DescriptorManager(Device& device);
	~DescriptorManager();

	void createDescriptorPool(uint32_t size);
	void createDescriptorSets(uint32_t size);
	void updateDescriptorSet(GameObject& gameObject,
		size_t bufferRangeSize, 
		VkDescriptorSet descriptorSet,
		VkBuffer uniformBuffer,
		VkImageView imageView);


	static std::vector<VkDescriptorSet> descriptorSets;
	static VkDescriptorSetLayout descriptorSetLayout;
private:
	Device& device;

	VkDescriptorPool descriptorPool;
	VkSampler textureSampler;

	void createDescriptorSetLayout();

	void createTextureSampler();
};
