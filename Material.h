#pragma once

class Shader;
class Graphics;

class Material
{
public:
	Material(Graphics* pGraphics);
	~Material();

	VkPipeline GetPipeline() { return m_Pipeline; }
	VkPipelineLayout GetPipelineLayout() { return m_PipelineLayout; }

	virtual void Load(const std::string& fileName);

	VkDescriptorSet GetDescriptorSet(const int index) const { return m_DescriptorSets[index]; }
	const std::vector<VkDescriptorSet>& GetDescriptorSets() const { return m_DescriptorSets; }
	
protected:
	Graphics * m_pGraphics;

	VkPipeline m_Pipeline;
	VkPipelineLayout m_PipelineLayout;
	VkDescriptorSetLayout m_DescriptorSetLayout;
	VkRenderPass m_RenderPass;
	VkDescriptorPool m_DescriptorPool;
	std::vector<VkDescriptorSet> m_DescriptorSets;
	std::vector<std::unique_ptr<Shader>> m_Shaders;

	void GetTypeAndSizeFromString(const std::string& type, VkFormat& format, int& size);
	VkShaderStageFlagBits GetShaderStageFromString(const std::string& stage);
};