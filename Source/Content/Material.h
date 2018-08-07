#pragma once

class Shader;
class Graphics;
class Texture2D;

class Material
{
public:
	Material(Graphics* pGraphics);
	~Material();

	VkPipeline GetPipeline() { return m_Pipeline; }
	virtual void Load(const std::string& fileName);
	VkDescriptorSet GetDescriptorSet() { return m_DescriptorSet; }

protected:
	VkDescriptorSet m_DescriptorSet;
	Graphics * m_pGraphics;
	VkPipeline m_Pipeline;
	std::vector<std::unique_ptr<Shader>> m_Shaders;

	void GetTypeAndSizeFromString(const std::string& type, VkFormat& format, int& size);
	VkShaderStageFlagBits GetShaderStageFromString(const std::string& stage);

	std::map<int, std::unique_ptr<Texture2D>> m_Textures;
};