#pragma once
class Graphics;

class VertexBuffer
{
public:
	VertexBuffer(Graphics* pGraphics);
	~VertexBuffer();


	void SetSize(const int size, const bool dynamic = false);
	void SetData(const int size, const int offset, void* pData);

	const VkBuffer& GetBuffer() const { return m_Buffer; }

private:
	Graphics * m_pGraphics;
	VkBuffer m_Buffer;
	VkDeviceMemory m_Memory;

	int m_Size = 0;
	int m_BufferSize = 0;
};