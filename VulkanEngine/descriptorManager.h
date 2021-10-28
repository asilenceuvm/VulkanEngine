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
	void updateObjectDescriptorSet(GameObject& gameObject,
		size_t bufferRangeSize, 
		VkDescriptorSet descriptorSet,
		VkBuffer uniformBuffer,
		VkImageView imageView);
	void updateTerrainDescriptorSet(GameObject& gameObject,
		size_t bufferRangeSize,
		VkDescriptorSet descriptorSet,
		VkBuffer uniformBuffer,
		VkImageView imageView,
		VkImageView imageView2);

	struct DescriptorSets {
		std::vector<VkDescriptorSet> objects;
		VkDescriptorSet terrain;
	};
	static DescriptorSets descriptorSets;
	struct DescriptorSetLayouts {
		VkDescriptorSetLayout terrain;
		VkDescriptorSetLayout object;
	};
	static DescriptorSetLayouts descriptorSetLayouts;
private:
	Device& device;

	VkDescriptorPool descriptorPool;
	VkSampler textureSampler;

	void createDescriptorSetLayouts();

	void createTextureSampler();
};
