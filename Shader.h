#pragma once
class Shader
{
public:
	Shader(VkDevice device);
	~Shader();

	bool Load(const std::string& filePath, VkShaderStageFlagBits shaderStage);

	VkShaderModule GetShaderObject();
	VkPipelineShaderStageCreateInfo& GetPipelineCreateInfo();

private:
	VkDevice m_Device;
	VkPipelineShaderStageCreateInfo m_PipelineInfo;
};