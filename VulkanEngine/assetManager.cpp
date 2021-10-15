#include "assetManager.h"

#include <filesystem>

#include "spdlog/spdlog.h"
#include "stb_image.h"

#include "utils.h"

std::vector<std::shared_ptr<Texture>> AssetManager::textures;

void AssetManager::loadTexture(Device& device, std::string filepath, bool flipped) {
	stbi_set_flip_vertically_on_load(flipped);
	std::shared_ptr<Texture> tex = std::make_shared<Texture>(device, filepath);
	textures.push_back(tex);
	spdlog::debug("Loaded {}", filepath);
}

void AssetManager::clearTextures() {
	textures.clear();
}
