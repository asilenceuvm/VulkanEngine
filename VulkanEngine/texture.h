#pragma once

#include <vulkan/vulkan.h>

#include "device.h"

class Texture {
public:
	Texture(Device& device) : device{ device } {};
	~Texture();

	void createTexture();
private:
	Device& device;

	VkImage textureImage;
	VkDeviceMemory textureImageMemory;
	VkBuffer stagingBuffer;
	VkDeviceMemory stagingBufferMemory;

	VkImageView textureImageView;


	void createImage(uint32_t width, uint32_t height, VkFormat format, VkImageTiling tiling, VkImageUsageFlags usage, VkMemoryPropertyFlags properties, VkImage& image, VkDeviceMemory& imageMemory);
	void transitionImageLayout(VkImage image, VkFormat format, VkImageLayout oldLayout, VkImageLayout newLayout);
	void createTextureImageView();
};

