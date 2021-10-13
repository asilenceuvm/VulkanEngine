#pragma once

#include <string>

#include <vulkan/vulkan.h>

#include "device.h"

class Texture {
public:
	Texture(Device& device, std::string filepath) : device{ device }{
		createTexture(device, filepath);
	};
	~Texture();

	void createTexture(Device& device, std::string filepath);

	VkImageView createTextureImageView(Device& device);
	static VkSampler createTextureSampler(Device& device);
private:
	Device& device;

	VkImage textureImage;
	VkDeviceMemory textureImageMemory;
	VkBuffer stagingBuffer;
	VkDeviceMemory stagingBufferMemory;

	VkImageView textureImageView;

	void createImage(Device& device, uint32_t width, uint32_t height, VkFormat format, VkImageTiling tiling, VkImageUsageFlags usage, VkMemoryPropertyFlags properties, VkImage& image, VkDeviceMemory& imageMemory);
	void transitionImageLayout(Device& device, VkImage image, VkFormat format, VkImageLayout oldLayout, VkImageLayout newLayout);
	VkImageView createImageView(Device& device, VkImage image, VkFormat format);
};

