#include "stdafx.h"
#include "Shader.h"

Shader::Shader(VkDevice device) :
	m_Device(device)
{
}

Shader::~Shader()
{
	vkDestroyShaderModule(m_Device, m_Module, nullptr);
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

	VkShaderModuleCreateInfo createInfo = {};
	createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
	createInfo.codeSize = bytes.size();
	createInfo.flags = 0;
	createInfo.pCode = (unsigned int*)bytes.data();
	createInfo.pNext = nullptr;
	if (vkCreateShaderModule(m_Device, &createInfo, nullptr, &m_Module) != VK_SUCCESS)
	{
		return false;
	}
	m_ShaderStage = shaderStage;
	return true;
}

VkShaderModule Shader::GetShaderObject()
{
	return m_Module;
}
