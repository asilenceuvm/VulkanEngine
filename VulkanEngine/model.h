#pragma once

#include <memory>
#include <vector>

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include "glm/glm.hpp"

#include "device.h"
#include "buffer.h"
#include "texture.h"

class Model {
public:
	struct Vertex {
		glm::vec3 position{};
		glm::vec3 color{};
		glm::vec3 normal{};
		glm::vec2 texCoord;

		static std::vector<VkVertexInputBindingDescription> getBindingDescriptions();
		static std::vector<VkVertexInputAttributeDescription> getAttributeDescriptions();

		bool operator==(const Vertex &other) const {
			return position == other.position && normal == other.normal && color == other.color && texCoord == other.texCoord;
		}
	};

	struct Geometry {
		std::vector<Vertex> vertices{};
		std::vector<uint32_t> indices{};

		void loadModel(const std::string& filepath);
	};

	Geometry geo;

	Model(Device &device, const Model::Geometry& geometry, std::shared_ptr<Texture> texture);
	~Model();

	//delete copy constructors
	Model(const Model&) = delete;
	Model& operator=(const Model&) = delete;

	void bind(VkCommandBuffer commandBuffer);
	void draw(VkCommandBuffer commandBuffer);

	static std::unique_ptr<Model> createModelFromFile(Device& device, const std::string& filepath, std::shared_ptr<Texture> texture);
	Geometry getGeometry();

	std::shared_ptr<Texture> getTexture() { return texture; }
private:
	Device& device;

	std::unique_ptr<Buffer> vertexBuffer;
	int32_t vertexCount;

	bool hasIndexBuffer = false;
	std::unique_ptr<Buffer> indexBuffer;
	uint32_t indexCount;

	std::shared_ptr<Texture> texture;


	void createVertexBuffers(const std::vector<Vertex> &vertices);
	void createIndexBuffers(const std::vector<uint32_t>& indices);
};

