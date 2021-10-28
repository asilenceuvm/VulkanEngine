#include "descriptorManager.h"

#include <array>

#include "spdlog/spdlog.h"

#include "engine.h"
#include "constants.h"

DescriptorManager::DescriptorSetLayouts DescriptorManager::descriptorSetLayouts;
DescriptorManager::DescriptorSets DescriptorManager::descriptorSets;

DescriptorManager::DescriptorManager(Device& device) : device{ device } {
	createDescriptorSetLayouts();
	createTextureSampler();
}

DescriptorManager::~DescriptorManager() {
	vkDestroySampler(device.device(), textureSampler, nullptr);
	vkDestroyDescriptorSetLayout(device.device(), descriptorSetLayouts.object, nullptr);
	vkDestroyDescriptorSetLayout(device.device(), descriptorSetLayouts.terrain, nullptr);
	vkDestroyDescriptorPool(device.device(), descriptorPool, nullptr);
}

void DescriptorManager::createDescriptorPool(uint32_t size) {
	std::array<VkDescriptorPoolSize, 2> poolSizes{};
	poolSizes[0].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	poolSizes[0].descriptorCount = size;
	poolSizes[1].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	poolSizes[1].descriptorCount = size;

	VkDescriptorPoolCreateInfo poolInfo{};
	poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
	poolInfo.poolSizeCount = static_cast<uint32_t>(poolSizes.size());
	poolInfo.pPoolSizes = poolSizes.data();
	poolInfo.maxSets = size;

	if (vkCreateDescriptorPool(device.device(), &poolInfo, nullptr, &descriptorPool) != VK_SUCCESS) {
		spdlog::critical("Failed to create descriptor pool");
	}
}

void DescriptorManager::createDescriptorSetLayouts() {
	std::vector<VkDescriptorSetLayoutBinding> setLayoutBindings;

	//terrain layout
	setLayoutBindings = {
		VkDescriptorSetLayoutBinding{0,			//binding
			VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,	//type
			1,									//count
			VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_TESSELLATION_CONTROL_BIT | VK_SHADER_STAGE_TESSELLATION_EVALUATION_BIT},		//flags
		VkDescriptorSetLayoutBinding{1,
			VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
			1,
			VK_SHADER_STAGE_TESSELLATION_CONTROL_BIT | VK_SHADER_STAGE_TESSELLATION_EVALUATION_BIT| VK_SHADER_STAGE_FRAGMENT_BIT,
			},
		VkDescriptorSetLayoutBinding{2,
			VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
			1,	
			VK_SHADER_STAGE_TESSELLATION_CONTROL_BIT | VK_SHADER_STAGE_TESSELLATION_EVALUATION_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
		},
	};

	VkDescriptorSetLayoutCreateInfo layoutInfo{};
	layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
	layoutInfo.bindingCount = static_cast<uint32_t>(setLayoutBindings.size());
	layoutInfo.pBindings = setLayoutBindings.data();

	if (vkCreateDescriptorSetLayout(device.device(), &layoutInfo, nullptr, &descriptorSetLayouts.terrain) != VK_SUCCESS) {
		spdlog::critical("Failed to create descriptor set layout");
	}

	setLayoutBindings = {
		VkDescriptorSetLayoutBinding{0,			//binding
			VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,	//type
			1,
			VK_SHADER_STAGE_VERTEX_BIT,			//stage flags
			},									//count
		VkDescriptorSetLayoutBinding{1,
			VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
			1,
			VK_SHADER_STAGE_FRAGMENT_BIT,
			},
	};

	VkDescriptorSetLayoutCreateInfo layoutInfo2{};
	layoutInfo2.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
	layoutInfo2.bindingCount = static_cast<uint32_t>(setLayoutBindings.size());
	layoutInfo2.pBindings = setLayoutBindings.data();

	if (vkCreateDescriptorSetLayout(device.device(), &layoutInfo2, nullptr, &descriptorSetLayouts.object) != VK_SUCCESS) {
		spdlog::critical("Failed to create descriptor set layout");
	}
}

void DescriptorManager::createDescriptorSets(uint32_t size) {
	std::vector<VkDescriptorSetLayout> layouts(size, descriptorSetLayouts.object);
	VkDescriptorSetAllocateInfo allocInfo{};
	allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
	allocInfo.descriptorPool = descriptorPool;
	allocInfo.descriptorSetCount = size - 1;
	allocInfo.pSetLayouts = layouts.data();

	descriptorSets.objects.resize(size);
	if (vkAllocateDescriptorSets(device.device(), &allocInfo, descriptorSets.objects.data()) != VK_SUCCESS) {
		throw std::runtime_error("Failed to allocate descriptor sets");
	}

	std::vector<VkDescriptorSetLayout> layouts2(size, descriptorSetLayouts.terrain);
	VkDescriptorSetAllocateInfo allocInfo2{};
	allocInfo2.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
	allocInfo2.descriptorPool = descriptorPool;
	allocInfo2.descriptorSetCount = 1;
	allocInfo2.pSetLayouts = layouts2.data();

	if (vkAllocateDescriptorSets(device.device(), &allocInfo2, &descriptorSets.terrain) != VK_SUCCESS) {
		throw std::runtime_error("Failed to allocate descriptor sets");
	}

}

void DescriptorManager::updateTerrainDescriptorSet(GameObject& gameObject,
	size_t bufferRangeSize,
	VkDescriptorSet descriptorSet,
	VkBuffer uniformBuffer,
	VkImageView imageView,
	VkImageView imageView2) {
	VkDescriptorBufferInfo bufferInfo{};
	bufferInfo.buffer = uniformBuffer;
	bufferInfo.offset = 0;
	bufferInfo.range = bufferRangeSize;

	VkDescriptorImageInfo imageInfo{};
	imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
	imageInfo.sampler = textureSampler;
	imageInfo.imageView = imageView;

	VkDescriptorImageInfo imageInfo2{};
	imageInfo2.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
	imageInfo2.sampler = textureSampler;
	imageInfo2.imageView = imageView2;

	std::array<VkWriteDescriptorSet, 3> descriptorWrites{};

	descriptorWrites[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	descriptorWrites[0].dstSet = descriptorSet;
	descriptorWrites[0].dstBinding = 0;
	descriptorWrites[0].dstArrayElement = 0;
	descriptorWrites[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	descriptorWrites[0].descriptorCount = 1;
	descriptorWrites[0].pBufferInfo = &bufferInfo;

	descriptorWrites[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	descriptorWrites[1].dstSet = descriptorSet;
	descriptorWrites[1].dstBinding = 1;
	descriptorWrites[1].dstArrayElement = 0;
	descriptorWrites[1].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	descriptorWrites[1].descriptorCount = 1;
	descriptorWrites[1].pImageInfo = &imageInfo;

	descriptorWrites[2].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	descriptorWrites[2].dstSet = descriptorSet;
	descriptorWrites[2].dstBinding = 2;
	descriptorWrites[2].dstArrayElement = 0;
	descriptorWrites[2].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	descriptorWrites[2].descriptorCount = 1;
	descriptorWrites[2].pImageInfo = &imageInfo2;

	vkUpdateDescriptorSets(device.device(), static_cast<uint32_t>(descriptorWrites.size()), descriptorWrites.data(), 0, nullptr);
}

void DescriptorManager::updateObjectDescriptorSet(GameObject& gameObject, 
		size_t bufferRangeSize, 
		VkDescriptorSet descriptorSet,
		VkBuffer uniformBuffer,
		VkImageView imageView) {
	VkDescriptorBufferInfo bufferInfo{};
	bufferInfo.buffer = uniformBuffer;
	bufferInfo.offset = 0;
	bufferInfo.range = bufferRangeSize;

	VkDescriptorImageInfo imageInfo{};
	imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
	imageInfo.sampler = textureSampler; 
	imageInfo.imageView = imageView;

	std::array<VkWriteDescriptorSet, 2> descriptorWrites{};

	descriptorWrites[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	descriptorWrites[0].dstSet = descriptorSet;
	descriptorWrites[0].dstBinding = 0;
	descriptorWrites[0].dstArrayElement = 0;
	descriptorWrites[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	descriptorWrites[0].descriptorCount = 1;
	descriptorWrites[0].pBufferInfo = &bufferInfo;

	descriptorWrites[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	descriptorWrites[1].dstSet = descriptorSet;
	descriptorWrites[1].dstBinding = 1;
	descriptorWrites[1].dstArrayElement = 0;
	descriptorWrites[1].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	descriptorWrites[1].descriptorCount = 1;
	descriptorWrites[1].pImageInfo = &imageInfo;

 
	vkUpdateDescriptorSets(device.device(), static_cast<uint32_t>(descriptorWrites.size()), descriptorWrites.data(), 0, nullptr);

}


void DescriptorManager::createTextureSampler() {
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
