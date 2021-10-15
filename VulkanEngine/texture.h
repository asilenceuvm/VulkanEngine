#pragma once

#include <string>

#include <vulkan/vulkan.h>

#include "device.h"

class Texture {
public:
	Texture(Device& device, std::string filepath) : device{ device }{
		createTexture(filepath);
		createTextureImageView();
	};
	~Texture();

	void createTexture(std::string filepath);
	void createTextureImageView();

	VkImageView getImageView() {
		return textureImageView;
	}
private:
	Device& device;

	VkImage textureImage;
	VkDeviceMemory textureImageMemory;
	VkBuffer stagingBuffer;
	VkDeviceMemory stagingBufferMemory;

	VkImageView textureImageView;

	void createImage(uint32_t width, uint32_t height, VkFormat format, VkImageTiling tiling, VkImageUsageFlags usage, VkMemoryPropertyFlags properties, VkImage& image, VkDeviceMemory& imageMemory);
	void transitionImageLayout(VkImage image, VkFormat format, VkImageLayout oldLayout, VkImageLayout newLayout);
	VkImageView createImageView(VkImage image, VkFormat format);
};

