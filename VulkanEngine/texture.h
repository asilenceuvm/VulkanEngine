#pragma once

#include <string>

#include <vulkan/vulkan.h>

#include "device.h"

class Texture {
public:
	Texture(Device& device) : device{ device } {};
	~Texture();

	static void createTexture(Device& device, std::string filepath);

	static VkImageView createTextureImageView(Device& device);
	static VkSampler createTextureSampler(Device& device);
private:
	Device& device;

	static VkImage textureImage;
	static VkDeviceMemory textureImageMemory;
	VkBuffer stagingBuffer;
	VkDeviceMemory stagingBufferMemory;

	static VkImageView textureImageView;
	static VkSampler textureSampler;


	static void createImage(Device& device, uint32_t width, uint32_t height, VkFormat format, VkImageTiling tiling, VkImageUsageFlags usage, VkMemoryPropertyFlags properties, VkImage& image, VkDeviceMemory& imageMemory);
	static void transitionImageLayout(Device& device, VkImage image, VkFormat format, VkImageLayout oldLayout, VkImageLayout newLayout);
	static VkImageView createImageView(Device& device, VkImage image, VkFormat format);
};

