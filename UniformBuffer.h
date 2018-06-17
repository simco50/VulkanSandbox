#pragma once
class Graphics;

class UniformBuffer
{
public:
	UniformBuffer(Graphics* pGraphics);
	~UniformBuffer();

	bool SetSize(const int size);
	bool SetData(const int offset, const int size, void* pData);

	int GetSize() const { return m_Size; }
	VkBuffer GetBuffer() const { return m_Buffer; }

private:
	Graphics * m_pGraphics;
	VkBuffer m_Buffer;
	VkDeviceMemory m_Memory;
	int m_Size = 0;
	int m_BufferSize = 0;
};