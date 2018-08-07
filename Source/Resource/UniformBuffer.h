#pragma once
class Graphics;

class UniformBuffer
{
public:
	UniformBuffer(Graphics* pGraphics);
	~UniformBuffer();

	void* Map();
	void Unmap();

	bool SetSize(const int size, const int maxRenames);
	bool SetData(const int offset, const int size, void* pData);

	int GetSize() const { return m_BufferSize; }
	VkBuffer GetBuffer() const { return m_Buffer; }
	int GetOffset(int objectIndex) const;

	void Flush();

private:
	Graphics * m_pGraphics;
	VkBuffer m_Buffer;

	void* m_pCurrentTarget = nullptr;
	void* m_pDataBegin = nullptr;

	int m_BufferSize = 0;
	int m_Renames = 0;
	int m_Stride = 0;
};