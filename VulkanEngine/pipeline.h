#pragma once

#include <string>
#include <vector>

#include "device.h"

struct PipelineConfigInfo {
	VkPipelineViewportStateCreateInfo viewportInfo;
	VkPipelineInputAssemblyStateCreateInfo inputAssemblyInfo;
	VkPipelineRasterizationStateCreateInfo rasterizationInfo;
	VkPipelineMultisampleStateCreateInfo multisampleInfo;
	VkPipelineColorBlendAttachmentState colorBlendAttachment;
	VkPipelineColorBlendStateCreateInfo colorBlendInfo;
	VkPipelineDepthStencilStateCreateInfo depthStencilInfo;
	std::vector<VkDynamicState> dynamicStateEnables;
	VkPipelineDynamicStateCreateInfo dynamicStateInfo;
	VkPipelineLayout pipelineLayout = nullptr;
	VkRenderPass renderPass = nullptr;
	uint32_t subpass = 0;
};
 
class Pipeline {
public:
	Pipeline(Device& device, 
		const std::string& vertFilepath, 
		const std::string& fragFilepath, 
		const PipelineConfigInfo& configInfo, 
		const std::string& teseFilepath = "", 
		const std::string& tescFilepath = "");
	~Pipeline();

	//delete copy constructors
	Pipeline(const Pipeline&) = delete;
	Pipeline& operator=(const Pipeline&) = delete;

	static void defaultPipelineConfigInfo(PipelineConfigInfo& configInfo);
	void bind(VkCommandBuffer commandBuffer);
private:
	Device& device;
	VkPipeline graphicsPipeline;

	VkShaderModule vertShaderModule;
	VkShaderModule fragShaderModule;
	VkShaderModule teseShaderModule = nullptr;
	VkShaderModule tescShaderModule = nullptr;

	void createGraphicsPipeline(const std::string& vertFilepath, 
		const std::string& fragFilepath, 
		const PipelineConfigInfo& configInfo,
		const std::string& teseFilepath = "",
		const std::string& tescFIlepath = "");
	void createShaderModule(const std::vector<char>& code, VkShaderModule* shaderModule);

	//helper functions
	static std::vector<char> readFile(const std::string& filepath);
};

