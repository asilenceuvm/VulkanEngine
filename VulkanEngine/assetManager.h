#pragma once

#include <vector>
#include <string>
#include <memory>

#include "texture.h"


class AssetManager {
public:
	static std::vector<std::shared_ptr<Texture>> textures;

	static void loadTexture(Device& device, std::string filepath, bool flipped = false);
	static void clearTextures();
private:

};

