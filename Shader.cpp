#include "stdafx.h"
#include "Shader.h"

Shader::Shader(VkDevice device) :
	m_Device(device)
{
	memset(&m_PipelineInfo, 0, sizeof(VkPipelineShaderStageCreateInfo));
}

Shader::~Shader()
{
	vkDestroyShaderModule(m_Device, m_PipelineInfo.module, nullptr);
}

bool Shader::Load(const std::string& filePath, VkShaderStageFlagBits shaderStage)
{
	std::ifstream stream(filePath, std::ios::ate | std::ios::binary);
	if (stream.fail())
	{
		return false;
	}
	std::vector<char> bytes((size_t)stream.tellg());
	stream.seekg(0);
	stream.read(bytes.data(), bytes.size());

	m_PipelineInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	m_PipelineInfo.flags = 0;
	m_PipelineInfo.pName = "main";
	m_PipelineInfo.pNext = nullptr;
	m_PipelineInfo.pSpecializationInfo = nullptr;
	m_PipelineInfo.stage = shaderStage;

	VkShaderModuleCreateInfo createInfo = {};
	createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
	createInfo.codeSize = bytes.size();
	createInfo.flags = 0;
	createInfo.pCode = (unsigned int*)bytes.data();
	createInfo.pNext = nullptr;
	if (vkCreateShaderModule(m_Device, &createInfo, nullptr, &m_PipelineInfo.module) != VK_SUCCESS)
	{
		return false;
	}
	return true;
}

VkShaderModule Shader::GetShaderObject()
{
	return m_PipelineInfo.module;
}

VkPipelineShaderStageCreateInfo& Shader::GetPipelineCreateInfo()
{
	return m_PipelineInfo;
}
