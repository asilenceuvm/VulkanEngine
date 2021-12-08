#include "model.h"

#include <cassert>
#include <cstring>
#include <unordered_map>


#define TINYOBJLOADER_IMPLEMENTATION
#include <tiny_obj_loader.h>
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/hash.hpp>
#include "spdlog/spdlog.h"
#include "stb_image.h"


#include "utils.h"
#include "assetManager.h"

template <>
struct std::hash<Model::Vertex> {
	size_t operator()(Model::Vertex const& vertex) const {
		size_t seed = 0;
		Utils::hashCombine(seed, vertex.position, vertex.color, vertex.normal, vertex.texCoord);
		return seed;
	}
};

Model::Model(Device& device, const Model::Geometry& geometry, std::shared_ptr<Texture> texture) : device{ device }, texture{ texture } {
	createVertexBuffers(geometry.vertices);
	createIndexBuffers(geometry.indices);
}
Model::~Model() {}

void Model::draw(VkCommandBuffer commandBuffer) {
	if (hasIndexBuffer) {
		vkCmdDrawIndexed(commandBuffer, indexCount, 1, 0, 0, 0);
	}
	else {
		vkCmdDraw(commandBuffer, vertexCount, 1, 0, 0);
	}
}

void Model::bind(VkCommandBuffer commandBuffer) {
	VkBuffer buffers[] = { vertexBuffer->getBuffer() };
	VkDeviceSize offsets[] = { 0 };
	vkCmdBindVertexBuffers(commandBuffer, 0, 1, buffers, offsets);
	if (hasIndexBuffer) {
		vkCmdBindIndexBuffer(commandBuffer, indexBuffer->getBuffer(), 0, VK_INDEX_TYPE_UINT32);
	}
}

std::unique_ptr<Model> Model::createModelFromFile(Device& device, const std::string& filepath, std::shared_ptr<Texture> texture) {
	Geometry geometry{};
	geometry.loadModel(filepath);
	return std::make_unique<Model>(device, geometry, texture);
}

void Model::createVertexBuffers(const std::vector<Vertex>& vertices) {
	vertexCount = static_cast<uint32_t>(vertices.size());
	VkDeviceSize bufferSize = sizeof(vertices[0]) * vertexCount;
	uint32_t vertexSize = sizeof(vertices[0]);

	Buffer stagingBuffer {
		device, vertexSize, static_cast<uint32_t>(vertexCount),
		VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
		VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT
	};
	stagingBuffer.map();
	stagingBuffer.writeToBuffer((void*)vertices.data());


	vertexBuffer = std::make_unique<Buffer>(
		device, vertexSize, vertexCount,
		VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
		VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT
	);

	device.copyBuffer(stagingBuffer.getBuffer(), vertexBuffer->getBuffer(), bufferSize);
}

void Model::createIndexBuffers(const std::vector<uint32_t>& indices) {
	indexCount = static_cast<uint32_t>(indices.size());
	hasIndexBuffer = indexCount > 0;
	uint32_t indexSize = sizeof(indices[0]);

	if (!hasIndexBuffer) {
		return;
	}

	VkDeviceSize bufferSize = sizeof(indices[0]) * indexCount;

	Buffer stagingBuffer{
		device, indexSize, indexCount,
		VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
		VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT
	};

	stagingBuffer.map();
	stagingBuffer.writeToBuffer((void*)indices.data());

	indexBuffer = std::make_unique<Buffer>(
		device, indexSize, indexCount,
		VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
		VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT
	);

	device.copyBuffer(stagingBuffer.getBuffer(), indexBuffer->getBuffer(), bufferSize);
}

std::vector<VkVertexInputBindingDescription> Model::Vertex::getBindingDescriptions() {
	std::vector<VkVertexInputBindingDescription> bindingDescriptions(1);
	bindingDescriptions[0].binding = 0;
	bindingDescriptions[0].stride = sizeof(Vertex);
	bindingDescriptions[0].inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
	return bindingDescriptions;
}
std::vector<VkVertexInputAttributeDescription> Model::Vertex::getAttributeDescriptions() {
	std::vector<VkVertexInputAttributeDescription> attributeDescriptions{};

	attributeDescriptions.push_back({0, 0, VK_FORMAT_R32G32B32_SFLOAT, offsetof(Vertex, position)});
	attributeDescriptions.push_back({1, 0, VK_FORMAT_R32G32B32_SFLOAT, offsetof(Vertex, color)});
	attributeDescriptions.push_back({2, 0, VK_FORMAT_R32G32B32_SFLOAT, offsetof(Vertex, normal)});
	attributeDescriptions.push_back({3, 0, VK_FORMAT_R32G32_SFLOAT, offsetof(Vertex, texCoord)});

	return attributeDescriptions;
}

void Model::Geometry::loadModel(const std::string& filepath) {
	tinyobj::attrib_t attrib;
	std::vector<tinyobj::shape_t> shapes;
	std::vector<tinyobj::material_t> materials;
	std::string warn, err;

	if (!tinyobj::LoadObj(&attrib, &shapes, &materials, &warn, &err, filepath.c_str())) {
		throw std::runtime_error(warn + err);
	}

	vertices.clear();
	indices.clear();

	std::unordered_map<Vertex, uint32_t> uniqueVertices{};
	for (const auto& shape : shapes) {
		for (const auto& index : shape.mesh.indices) {
			Vertex vertex{};

			if (index.vertex_index >= 0) {
				vertex.position = {
					attrib.vertices[3 * index.vertex_index + 0],
					attrib.vertices[3 * index.vertex_index + 1],
					attrib.vertices[3 * index.vertex_index + 2],
				};

				auto colorIndex = 3 * index.vertex_index + 2;
				if (colorIndex < attrib.colors.size()) {
					vertex.color = {
						attrib.colors[colorIndex - 2],
						attrib.colors[colorIndex - 1],
						attrib.colors[colorIndex - 0],
					};
				}
				else {
					vertex.color = { 1.f, 1.f, 1.f };  // set default color
				}
			}
			if (index.normal_index >= 0) {
				vertex.normal = {
					attrib.normals[3 * index.normal_index + 0],
					attrib.normals[3 * index.normal_index + 1],
					attrib.normals[3 * index.normal_index + 2],
				};
			}

			if (index.texcoord_index >= 0) {
				vertex.texCoord = {
					attrib.texcoords[2 * index.texcoord_index + 0],
					1.0f - attrib.texcoords[2 * index.texcoord_index + 1]
				};
			}

			if (uniqueVertices.count(vertex) == 0) {
				uniqueVertices[vertex] = static_cast<uint32_t>(vertices.size());
				vertices.push_back(vertex);
			}
			indices.push_back(uniqueVertices[vertex]);
		}
	}
}

Model::HeightMap::HeightMap(Device& device, std::string filename, uint32_t patchsize) {
	int texWidth, texHeight, texChannels;
	stbi_uc* pixels = stbi_load(filename.c_str(), &texWidth, &texHeight, &texChannels, STBI_rgb_alpha);
	VkDeviceSize imageSize = texWidth * texHeight * 4;

	if (!pixels) {
		spdlog::critical("Failed to load texture image");
	}

	VkBuffer stagingBuffer;
	VkDeviceMemory stagingBufferMemory;
	device.createBuffer(imageSize, 
		VK_BUFFER_USAGE_TRANSFER_SRC_BIT, 
		VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, 
		stagingBuffer, 
		stagingBufferMemory);

	void* data;
	vkMapMemory(device.device(), stagingBufferMemory, 0, imageSize, 0, &data);
		memcpy(data, pixels, static_cast<size_t>(imageSize));
	vkUnmapMemory(device.device(), stagingBufferMemory);

	dim = texWidth;
	heightdata = new uint16_t[dim * dim];
	this->scale = dim / patchsize;
	//this->scale = 2;

	stbi_image_free(pixels);
    vkDestroyBuffer(device.device(), stagingBuffer, nullptr);
    vkFreeMemory(device.device(), stagingBufferMemory, nullptr);
};

float Model::HeightMap::getHeight(uint32_t x, uint32_t y) {
	glm::ivec2 rpos = glm::ivec2(x, y) * glm::ivec2(scale);
	rpos.x = std::max(0, std::min(rpos.x, (int)dim-1));
	rpos.y = std::max(0, std::min(rpos.y, (int)dim-1));
	rpos /= glm::ivec2(scale);
	return *(heightdata + (rpos.x + rpos.y * dim) * scale) / 65535.0f;
}

// Generate a terrain quad patch for feeding to the tessellation control shader
std::unique_ptr<Model> Model::generateTerrain(Device& device, float patchSize, float uvScale) {
#define PATCH_SIZE 64
	const uint32_t vertexCount = PATCH_SIZE * PATCH_SIZE;
	//Model::Vertex* vertices = new Model::Vertex[vertexCount];
	std::vector<Model::Vertex> vertices;
	vertices.resize(vertexCount);


	const float wx = 2.0f;
	const float wy = 2.0f;

	for (auto x = 0; x < PATCH_SIZE; x++) {
		for (auto y = 0; y < PATCH_SIZE; y++) {
			uint32_t index = (x + y * PATCH_SIZE);
			vertices[index].position[0] = x * wx + wx / 2.0f - (float)PATCH_SIZE * wx / 2.0f;
			vertices[index].position[1] = 0.0f;
			vertices[index].position[2] = y * wy + wy / 2.0f - (float)PATCH_SIZE * wy / 2.0f;
			vertices[index].texCoord = glm::vec2((float)x / PATCH_SIZE, (float)y / PATCH_SIZE);
		//	vertices[index].texCoord = glm::vec2(x % 4, y % 4);
			vertices[index].color = glm::vec3(1);
			vertices[index].normal = glm::vec3(1);
		}
	}

	// Calculate normals from height map using a sobel filter
	Model::HeightMap heightMap(device, "textures/heightmap.png", PATCH_SIZE);
	for (auto x = 0; x < PATCH_SIZE; x++) {
		for (auto y = 0; y < PATCH_SIZE; y++) {
			float heights[3][3];
			for (auto hx = -1; hx <= 1; hx++) {
				for (auto hy = -1; hy <= 1; hy++) {
					heights[hx + 1][hy + 1] = heightMap.getHeight(x + hx, y + hy);
				}
			}

			// Calculate the normal
			glm::vec3 normal;
			normal.x = heights[0][0] - heights[2][0] + 2.0f * heights[0][1] - 2.0f * heights[2][1] + heights[0][2] - heights[2][2];
			normal.z = heights[0][0] + 2.0f * heights[1][0] + heights[2][0] - heights[0][2] - 2.0f * heights[1][2] - heights[2][2];
			normal.y = 0.25f * sqrt(1.0f - normal.x * normal.x - normal.z * normal.z);

			vertices[x + y * (int)PATCH_SIZE].normal = glm::normalize(normal * glm::vec3(2.0f, 1.0f, 2.0f));
		}
	}

	// Indices
	const uint32_t w = (PATCH_SIZE - 1);
	const uint32_t indexCount = w * w * 4;
	uint32_t* indices = new uint32_t[indexCount];
	for (auto x = 0; x < w; x++) {
		for (auto y = 0; y < w; y++) {
			uint32_t index = (x + y * w) * 4;
			indices[index] = (x + y * PATCH_SIZE);
			indices[index + 1] = indices[index] + PATCH_SIZE;
			indices[index + 2] = indices[index + 1] + 1;
			indices[index + 3] = indices[index] + 1;
		}
	}
	Model::Geometry geometry;
	//geometry.vertices = std::vector<Model::Vertex>(vertices, vertices + vertexCount);
	geometry.vertices = vertices;
	geometry.indices = std::vector<uint32_t>(indices, indices + indexCount);

	//delete[] vertices;
	delete[] indices;

	return std::make_unique<Model>(device, geometry, AssetManager::textures["sand"]);
}
std::unique_ptr<Model> Model::generateMesh(Device& device, int length, int width, std::shared_ptr<Texture> texture, std::string heightmap) {
	Model::Geometry geometry;

	int total = 0;
	//generate length * width squares
	for (int i = 0; i < length; i++) {
		for (int j = 0; j < width; j++) {
			float yPos[4] = { 0.0f, 0.0f, 0.0f, 0.0f };
			Model::Vertex vert1{ {i - .5f, yPos[0], j - .5f}, {1.f, 1.f, 1.f}, {.0f, 1.f, 0.0f}, {1.f, 0.f} };
			Model::Vertex vert2{ {i - .5f, yPos[1], j + .5f}, {1.f, 1.f, 1.f}, {.0f, 1.f, 0.0f}, {0.f, 0.f} };
			Model::Vertex vert3{ {i + .5f, yPos[2], j + .5f}, {1.f, 1.f, 1.f}, {.0f, 1.f, 0.0f}, {0.f, 1.f} };
			Model::Vertex vert4{ {i + .5f, yPos[3], j - .5f}, {1.f, 1.f, 1.f}, {.0f, 1.f, 0.0f}, {1.f, 1.f} };

			geometry.vertices.push_back(vert1);
			geometry.vertices.push_back(vert2);
			geometry.vertices.push_back(vert3);
			geometry.vertices.push_back(vert4);

			//0, 1, 2, 2, 3, 0
			geometry.indices.push_back((total) * 4);
			geometry.indices.push_back(1 + (total) * 4);
			geometry.indices.push_back(2 + (total) * 4);
			geometry.indices.push_back(2 + (total) * 4);
			geometry.indices.push_back(3 + (total) * 4);
			geometry.indices.push_back((total) * 4);
			total++;
		}
	}

	return std::make_unique<Model>(device, geometry, texture);
}

