#include "pipeline.h"

#include <fstream>
#include <stdexcept>
#include <cassert>

#include "spdlog/spdlog.h"
#include "model.h"


Pipeline::Pipeline(Device& device, 
		const std::string& vertFilepath, 
		const std::string& fragFilepath, 
		const PipelineConfigInfo& configInfo,
		const std::string& teseFilepath, 
		const std::string& tescFilepath) : device(device){
	createGraphicsPipeline(vertFilepath, fragFilepath, configInfo);
}

Pipeline::~Pipeline() {
	vkDestroyShaderModule(device.device(), vertShaderModule, nullptr);
	vkDestroyShaderModule(device.device(), fragShaderModule, nullptr);
	vkDestroyPipeline(device.device(), graphicsPipeline, nullptr);
}

void Pipeline::createGraphicsPipeline(const std::string& vertFilepath, 
		const std::string& fragFilepath, 
		const PipelineConfigInfo& configInfo,
		const std::string& teseFilepath, 
		const std::string& tescFilepath) {
	if (configInfo.pipelineLayout == VK_NULL_HANDLE) {
		spdlog::critical("Failed to find pipelinelayout in config info");
		throw std::runtime_error("createGraphicsPipeline");
	}
	if (configInfo.renderPass == VK_NULL_HANDLE) {
		spdlog::critical("Failed to find renderpass in config info");
		throw std::runtime_error("createGraphicsPipeline");
	}


	std::vector<VkPipelineShaderStageCreateInfo> shaderStages;
	auto vertCode = readFile(vertFilepath);
	auto fragCode = readFile(fragFilepath);
	createShaderModule(vertCode, &vertShaderModule);
	createShaderModule(fragCode, &fragShaderModule);

	VkPipelineShaderStageCreateInfo vertShaderStage{};
	vertShaderStage.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	vertShaderStage.stage = VK_SHADER_STAGE_VERTEX_BIT;
	vertShaderStage.module = vertShaderModule;
	vertShaderStage.pName = "main";
	vertShaderStage.flags = 0;
	vertShaderStage.pNext = nullptr;
	vertShaderStage.pSpecializationInfo = nullptr;
	shaderStages.push_back(vertShaderStage);

	VkPipelineShaderStageCreateInfo fragShaderStage{};
	fragShaderStage.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	fragShaderStage.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
	fragShaderStage.module = fragShaderModule;
	fragShaderStage.pName = "main";
	fragShaderStage.flags = 0;
	fragShaderStage.pNext = nullptr;
	fragShaderStage.pSpecializationInfo = nullptr;
	shaderStages.push_back(fragShaderStage);

	if (teseFilepath != "") {
		auto tescCode = readFile(tescFilepath);
		auto teseCode = readFile(teseFilepath);
		createShaderModule(tescCode, &tescShaderModule);
		createShaderModule(teseCode, &teseShaderModule);

		VkPipelineShaderStageCreateInfo tescShaderStage{};
		tescShaderStage.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
		tescShaderStage.stage = VK_SHADER_STAGE_TESSELLATION_CONTROL_BIT;
		tescShaderStage.module = fragShaderModule;
		tescShaderStage.pName = "main";
		tescShaderStage.flags = 0;
		tescShaderStage.pNext = nullptr;
		tescShaderStage.pSpecializationInfo = nullptr;
		shaderStages.push_back(tescShaderStage);

		VkPipelineShaderStageCreateInfo teseShaderStage{};
		teseShaderStage.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
		teseShaderStage.stage = VK_SHADER_STAGE_TESSELLATION_EVALUATION_BIT;
		teseShaderStage.module = vertShaderModule;
		teseShaderStage.pName = "main";
		teseShaderStage.flags = 0;
		teseShaderStage.pNext = nullptr;
		teseShaderStage.pSpecializationInfo = nullptr;
		shaderStages.push_back(teseShaderStage);
	}

	auto bindingDescriptions = Model::Vertex::getBindingDescriptions();
	auto attributeDescriptions = Model::Vertex::getAttributeDescriptions();
	VkPipelineVertexInputStateCreateInfo vertexInputInfo{};
	vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
	vertexInputInfo.vertexAttributeDescriptionCount = static_cast<uint32_t>(attributeDescriptions.size());
	vertexInputInfo.vertexBindingDescriptionCount = static_cast<uint32_t>(bindingDescriptions.size());;
	vertexInputInfo.pVertexAttributeDescriptions = attributeDescriptions.data();
	vertexInputInfo.pVertexBindingDescriptions = bindingDescriptions.data();


	VkGraphicsPipelineCreateInfo pipelineInfo{};
	pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
	pipelineInfo.stageCount = shaderStages.size();
	pipelineInfo.pStages = shaderStages.data();
	pipelineInfo.pVertexInputState = &vertexInputInfo;
	pipelineInfo.pInputAssemblyState = &configInfo.inputAssemblyInfo;
	pipelineInfo.pViewportState = &configInfo.viewportInfo;
	pipelineInfo.pRasterizationState = &configInfo.rasterizationInfo;
	pipelineInfo.pMultisampleState = &configInfo.multisampleInfo;
	pipelineInfo.pColorBlendState = &configInfo.colorBlendInfo;
	pipelineInfo.pDepthStencilState = &configInfo.depthStencilInfo;
	pipelineInfo.pDynamicState = &configInfo.dynamicStateInfo;
	if (teseFilepath != "") {
		VkPipelineTessellationStateCreateInfo pipelineTessellationStateCreateInfo {};
		pipelineTessellationStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_TESSELLATION_STATE_CREATE_INFO;
		pipelineTessellationStateCreateInfo.patchControlPoints = 4;
		;
		pipelineInfo.pTessellationState = &pipelineTessellationStateCreateInfo;
	}

	pipelineInfo.layout = configInfo.pipelineLayout;
	pipelineInfo.renderPass = configInfo.renderPass;
	pipelineInfo.subpass = configInfo.subpass;

	pipelineInfo.basePipelineIndex = -1;
	pipelineInfo.basePipelineHandle = VK_NULL_HANDLE;

	if (vkCreateGraphicsPipelines(device.device(), VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &graphicsPipeline) != VK_SUCCESS) {
		spdlog::critical("Failed to create graphics pipeline");
		throw std::runtime_error("createPipeline");
	}
}

void Pipeline::createShaderModule(const std::vector<char>& code, VkShaderModule* shaderModule) {
	VkShaderModuleCreateInfo createInfo{};
	createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
	createInfo.codeSize = code.size();
	createInfo.pCode = reinterpret_cast<const uint32_t*> (code.data());

	if (vkCreateShaderModule(device.device(), &createInfo, nullptr, shaderModule) != VK_SUCCESS) {
		spdlog::critical("Failed to create shader module");
		throw std::runtime_error("createShaderModule");
	}
}

void Pipeline::defaultPipelineConfigInfo(PipelineConfigInfo& configInfo) {
	configInfo.inputAssemblyInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
	configInfo.inputAssemblyInfo.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
	configInfo.inputAssemblyInfo.primitiveRestartEnable = VK_FALSE;

	configInfo.viewportInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
	configInfo.viewportInfo.viewportCount = 1;
	configInfo.viewportInfo.pViewports = nullptr;
	configInfo.viewportInfo.scissorCount = 1;
	configInfo.viewportInfo.pScissors = nullptr;

	configInfo.rasterizationInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
	configInfo.rasterizationInfo.depthClampEnable = VK_FALSE;
	configInfo.rasterizationInfo.rasterizerDiscardEnable = VK_FALSE;
	configInfo.rasterizationInfo.polygonMode = VK_POLYGON_MODE_FILL;
	//configInfo.rasterizationInfo.polygonMode = VK_POLYGON_MODE_LINE;
	configInfo.rasterizationInfo.lineWidth = 1.0f;
	configInfo.rasterizationInfo.cullMode = VK_CULL_MODE_BACK_BIT;
	configInfo.rasterizationInfo.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
	configInfo.rasterizationInfo.depthBiasEnable = VK_FALSE;

	configInfo.multisampleInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
	configInfo.multisampleInfo.sampleShadingEnable = VK_FALSE;
	configInfo.multisampleInfo.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

	configInfo.colorBlendAttachment.colorWriteMask =
	  VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT |
	  VK_COLOR_COMPONENT_A_BIT;
	configInfo.colorBlendAttachment.blendEnable = VK_FALSE;
	configInfo.colorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_ONE;   
	configInfo.colorBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ZERO;  
	configInfo.colorBlendAttachment.colorBlendOp = VK_BLEND_OP_ADD;              
	configInfo.colorBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;   
	configInfo.colorBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;  
	configInfo.colorBlendAttachment.alphaBlendOp = VK_BLEND_OP_ADD;              

	configInfo.colorBlendInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
	configInfo.colorBlendInfo.logicOpEnable = VK_FALSE;
	configInfo.colorBlendInfo.logicOp = VK_LOGIC_OP_COPY;  
	configInfo.colorBlendInfo.attachmentCount = 1;
	configInfo.colorBlendInfo.pAttachments = &configInfo.colorBlendAttachment;
	configInfo.colorBlendInfo.blendConstants[0] = 0.0f;  
	configInfo.colorBlendInfo.blendConstants[1] = 0.0f;  
	configInfo.colorBlendInfo.blendConstants[2] = 0.0f;  
	configInfo.colorBlendInfo.blendConstants[3] = 0.0f;  

	configInfo.depthStencilInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
	configInfo.depthStencilInfo.depthTestEnable = VK_TRUE;
	configInfo.depthStencilInfo.depthWriteEnable = VK_TRUE;
	configInfo.depthStencilInfo.depthCompareOp = VK_COMPARE_OP_LESS;
	configInfo.depthStencilInfo.depthBoundsTestEnable = VK_FALSE;
	configInfo.depthStencilInfo.stencilTestEnable = VK_FALSE;

	configInfo.dynamicStateEnables = {VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR};
	configInfo.dynamicStateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
	configInfo.dynamicStateInfo.pDynamicStates = configInfo.dynamicStateEnables.data();
	configInfo.dynamicStateInfo.dynamicStateCount =
	  static_cast<uint32_t>(configInfo.dynamicStateEnables.size());
	configInfo.dynamicStateInfo.flags = 0;
}

void Pipeline::bind(VkCommandBuffer commandBuffer) {
	vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, graphicsPipeline);
}

std::vector<char> Pipeline::readFile(const std::string& filepath) {
	std::ifstream file{filepath, std::ios::ate | std::ios::binary};
	if (!file.is_open()) {
		spdlog::critical("Failed to open file {}", filepath);
		throw std::runtime_error("failed to open file: " + filepath);
    }

	size_t fileSize = static_cast<size_t>(file.tellg());
	std::vector<char> buffer(fileSize);

	file.seekg(0);
	file.read(buffer.data(), fileSize);

	file.close();
	return buffer;
}
