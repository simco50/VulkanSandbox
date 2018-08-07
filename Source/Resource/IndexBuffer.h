#pragma once
class Graphics;

class IndexBuffer
{
public:
	IndexBuffer(Graphics* pGraphics);
	~IndexBuffer();

	void SetSize(const int count, bool smallIndices, const bool dynamic = false);
	void SetData(void* pData);

	const VkBuffer& GetBuffer() const { return m_Buffer; }
	int GetCount() const { return m_IndexCount; }

private:
	Graphics * m_pGraphics;
	VkBuffer m_Buffer;
	VkDeviceMemory m_Memory;
	int m_IndexCount = 0;
	int m_IndexSize = 0;
	int m_Size = 0;
	int m_BufferSize = 0;
};