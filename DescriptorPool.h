#pragma once
class Graphics;

class DescriptorPool
{
public:
	DescriptorPool(Graphics* pGraphics);
	~DescriptorPool();

	VkDescriptorSet Allocate(VkDescriptorSetLayout layout);
	void Free(const VkDescriptorSet& set);
	void Free(const std::vector<VkDescriptorSet>& sets);

private:
	Graphics * m_pGraphics;
	VkDescriptorPool m_Pool;
};

