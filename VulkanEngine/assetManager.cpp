#include "assetManager.h"

#include <filesystem>

#include "spdlog/spdlog.h"
#include "stb_image.h"

#include "utils.h"

std::map<std::string, std::shared_ptr<Texture>> AssetManager::textures;

void AssetManager::loadTexture(Device& device, std::string filepath, std::string name, bool flipped) {
	stbi_set_flip_vertically_on_load(flipped);
	std::shared_ptr<Texture> tex = std::make_shared<Texture>(device, filepath);
	textures.insert(std::pair<std::string, std::shared_ptr<Texture>>(name, tex));
	spdlog::debug("Loaded {}", filepath);
}

void AssetManager::loadCubeMap(Device& device, std::array<std::string, 6> filepaths, std::string name, bool flipped) {
	stbi_set_flip_vertically_on_load(flipped);
	std::shared_ptr<Texture> tex = std::make_shared<Texture>(device, filepaths);
	textures.insert(std::pair<std::string, std::shared_ptr<Texture>>(name, tex));
	spdlog::debug("Loaded {}", filepaths[0]);
}

void AssetManager::clearTextures() {
	textures.clear();
}
