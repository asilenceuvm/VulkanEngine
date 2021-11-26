#pragma once

#include <string>
#include <memory>
#include <map>

#include "texture.h"
#include "model.h"


class AssetManager {
public:
	static std::map<std::string, std::shared_ptr<Texture>> textures;
	static std::map<std::string, std::shared_ptr<Model>> models;

	static void loadTexture(Device& device, std::string filepath, std::string name, bool flipped = false);
	static void loadCubeMap(Device& device, std::array<std::string, 6> filepaths, std::string name, bool flipped = false);
	static void clearTextures();

	static void loadModel(Device& device, std::string modelName, std::string modelFilepath, std::string textureFilepath);
	static void clearModels();
private:

};

