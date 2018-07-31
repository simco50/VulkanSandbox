#pragma once
class Shader
{
public:
	Shader(VkDevice device);
	~Shader();

	bool Load(const std::string& filePath, VkShaderStageFlagBits shaderStage);

	VkShaderModule GetShaderObject();
	VkShaderStageFlagBits GetStage() const { return m_ShaderStage; }

private:
	VkDevice m_Device;
	VkShaderStageFlagBits m_ShaderStage;
	VkShaderModule m_Module;
};