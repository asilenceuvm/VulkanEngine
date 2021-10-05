#pragma once

#include <memory>
#include <vector>

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include "glm/glm.hpp"

#include "device.h"
#include "buffer.h"

class Model {
public:
	struct Vertex {
		glm::vec3 position{};
		glm::vec3 color{};
		glm::vec3 normal{};
		glm::vec2 uv{};

		static std::vector<VkVertexInputBindingDescription> getBindingDescriptions();
		static std::vector<VkVertexInputAttributeDescription> getAttributeDescriptions();

		bool operator==(const Vertex &other) const {
			return position == other.position && color == other.color && normal == other.normal && uv == other.uv;
		}
	};

	struct Geometry {
		std::vector<Vertex> vertices{};
		std::vector<uint32_t> indices{};

		void loadModel(const std::string& filepath);
	};


	Model(Device &device, const Model::Geometry& geometry);
	~Model();

	//delete copy constructors
	Model(const Model&) = delete;
	Model& operator=(const Model&) = delete;

	void bind(VkCommandBuffer commandBuffer);
	void draw(VkCommandBuffer commandBuffer);

	static std::unique_ptr<Model> createModelFromFile(Device& device, const std::string& filepath);

private:
	Device& device;

	std::unique_ptr<Buffer> vertexBuffer;
	int32_t vertexCount;

	bool hasIndexBuffer = false;
	std::unique_ptr<Buffer> indexBuffer;
	uint32_t indexCount;


	void createVertexBuffers(const std::vector<Vertex> &vertices);
	void createIndexBuffers(const std::vector<uint32_t>& indices);
};

