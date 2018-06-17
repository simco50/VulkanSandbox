#pragma once
class Graphics;

class UniformBuffer
{
public:
	UniformBuffer(Graphics* pGraphics);
	~UniformBuffer();

	bool SetSize(const int size);
	bool SetData(const int offset, const int size, void* pData);
	const VkDescriptorBufferInfo& GetDescriptorInfo() const;

private:
	Graphics * m_pGraphics;
	VkDeviceMemory m_Memory;
	VkDescriptorBufferInfo m_DescriptorInfo = {};
	int m_Size = 0;
	int m_BufferSize = 0;
};