#include "model.h"

#include <cassert>
#include <cstring>
#include <unordered_map>


#define TINYOBJLOADER_IMPLEMENTATION
#include <tiny_obj_loader.h>
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/hash.hpp>


#include "utils.h"

template <>
struct std::hash<Model::Vertex> {
	size_t operator()(Model::Vertex const& vertex) const {
		size_t seed = 0;
		Utils::hashCombine(seed, vertex.position, vertex.color, vertex.normal, vertex.uv);
		return seed;
	}
};

Model::Model(Device& device, const Model::Geometry& geometry) : device{ device } {
	createVertexBuffers(geometry.vertices);
	createIndexBuffers(geometry.indices);
}
Model::~Model() {
	vkDestroyBuffer(device.device(), vertexBuffer, nullptr);
	vkFreeMemory(device.device(), vertexBufferMemory, nullptr);

	if (hasIndexBuffer) {
		vkDestroyBuffer(device.device(), indexBuffer, nullptr);
		vkFreeMemory(device.device(), indexBufferMemory, nullptr);
	}
}

void Model::draw(VkCommandBuffer commandBuffer) {
	if (hasIndexBuffer) {
		vkCmdDrawIndexed(commandBuffer, indexCount, 1, 0, 0, 0);
	}
	else {
		vkCmdDraw(commandBuffer, vertexCount, 1, 0, 0);
	}
}

void Model::bind(VkCommandBuffer commandBuffer) {
	VkBuffer buffers[] = { vertexBuffer };
	VkDeviceSize offsets[] = { 0 };
	vkCmdBindVertexBuffers(commandBuffer, 0, 1, buffers, offsets);
	if (hasIndexBuffer) {
		vkCmdBindIndexBuffer(commandBuffer, indexBuffer, 0, VK_INDEX_TYPE_UINT32);
	}
}

std::unique_ptr<Model> Model::createModelFromFile(Device& device, const std::string& filepath) {
	Geometry geometry{};
	geometry.loadModel(filepath);
	return std::make_unique<Model>(device, geometry);
}

void Model::createVertexBuffers(const std::vector<Vertex>& vertices) {
	vertexCount = static_cast<uint32_t>(vertices.size());
	assert(vertexCount >= 3 && "Vertex count must be at least 3");
	VkDeviceSize bufferSize = sizeof(vertices[0]) * vertexCount;

	VkBuffer stagingBuffer;
	VkDeviceMemory stagingBufferMemory;
	device.createBuffer(
		bufferSize,
		VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
		VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
		stagingBuffer,
		stagingBufferMemory);

	void* data;
	vkMapMemory(device.device(), stagingBufferMemory, 0, bufferSize, 0, &data);
	memcpy(data, vertices.data(), static_cast<size_t>(bufferSize));
	vkUnmapMemory(device.device(), stagingBufferMemory);

	device.createBuffer(
		bufferSize,
		VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
		VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
		vertexBuffer,
		vertexBufferMemory);

	device.copyBuffer(stagingBuffer, vertexBuffer, bufferSize);

	vkDestroyBuffer(device.device(), stagingBuffer, nullptr);
	vkFreeMemory(device.device(), stagingBufferMemory, nullptr);
}

void Model::createIndexBuffers(const std::vector<uint32_t>& indices) {
	indexCount = static_cast<uint32_t>(indices.size());
	hasIndexBuffer = indexCount > 0;

	if (!hasIndexBuffer) {
		return;
	}

	VkDeviceSize bufferSize = sizeof(indices[0]) * indexCount;

	VkBuffer stagingBuffer;
	VkDeviceMemory stagingBufferMemory;
	device.createBuffer(
		bufferSize,
		VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
		VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
		stagingBuffer,
		stagingBufferMemory);

	void* data;
	vkMapMemory(device.device(), stagingBufferMemory, 0, bufferSize, 0, &data);
	memcpy(data, indices.data(), static_cast<size_t>(bufferSize));
	vkUnmapMemory(device.device(), stagingBufferMemory);

	device.createBuffer(
		bufferSize,
		VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
		VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
		indexBuffer,
		indexBufferMemory);

	device.copyBuffer(stagingBuffer, indexBuffer, bufferSize);

	vkDestroyBuffer(device.device(), stagingBuffer, nullptr);
	vkFreeMemory(device.device(), stagingBufferMemory, nullptr);
}

std::vector<VkVertexInputBindingDescription> Model::Vertex::getBindingDescriptions() {
	std::vector<VkVertexInputBindingDescription> bindingDescriptions(1);
	bindingDescriptions[0].binding = 0;
	bindingDescriptions[0].stride = sizeof(Vertex);
	bindingDescriptions[0].inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
	return bindingDescriptions;
}
std::vector<VkVertexInputAttributeDescription> Model::Vertex::getAttributeDescriptions() {
	std::vector<VkVertexInputAttributeDescription> attributeDescriptions(2);
	attributeDescriptions[0].binding = 0;
	attributeDescriptions[0].location = 0;
	attributeDescriptions[0].format = VK_FORMAT_R32G32B32_SFLOAT;
	attributeDescriptions[0].offset = offsetof(Vertex, position);

	attributeDescriptions[1].binding = 0;
	attributeDescriptions[1].location = 1;
	attributeDescriptions[1].format = VK_FORMAT_R32G32B32_SFLOAT;
	attributeDescriptions[1].offset = offsetof(Vertex, color);

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
				vertex.uv = {
					attrib.texcoords[2 * index.texcoord_index + 0],
					attrib.texcoords[2 * index.texcoord_index + 1],
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
