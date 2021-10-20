#pragma once

#include <array>
#include <string>

#include <vulkan/vulkan.h>

#include "device.h"

class Texture {
public:
	Texture(Device& device, std::string filepath) : device{ device }{
		createTexture(filepath);
		createTextureImageView(1, VK_IMAGE_VIEW_TYPE_2D);
	};
	Texture(Device& device, std::array<std::string, 6> filepaths) : device{ device }{
		createCubemap(filepaths);
		createTextureImageView(6, VK_IMAGE_VIEW_TYPE_CUBE);
	};
	~Texture();


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

	void createTexture(std::string filepath);
	void createTextureImageView(uint32_t arrayLayers, VkImageViewType imageViewType);

	void createCubemap(std::array<std::string, 6> filepaths);

	void createImage(uint32_t width, 
		uint32_t height, 
		VkFormat format, 
		VkImageTiling tiling, 
		VkImageUsageFlags usage, 
		VkMemoryPropertyFlags properties, 
		uint32_t arrayLayers, 
		bool cube, 
		VkImage& image, 
		VkDeviceMemory& imageMemory);
	VkImageView createImageView(VkImage image, 
		uint32_t arrayLayers, 
		VkImageViewType imageViewType, 
		VkFormat format);
	void transitionImageLayout(VkImage image, 
		VkFormat format, 
		uint32_t arrayLayers, 
		VkImageLayout oldLayout, 
		VkImageLayout newLayout);
};

