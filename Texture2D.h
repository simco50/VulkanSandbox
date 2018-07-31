#pragma once
class Graphics;

class Texture2D
{
public:
	Texture2D(Graphics* pGraphics);
	~Texture2D();

	bool Load(const std::string& filePath);
	void SetSize(const int width, const int height, const unsigned int format, unsigned int usage, const int multiSample, int64 pTexture);
	bool SetData(const unsigned int mipLevel, int x, int y, int width, int height, const void* pData);
	void UpdateParameters();

	void SetLayout(VkCommandBuffer cmdBuffer, VkImageAspectFlags aspectMask, VkImageLayout newImageLayout, VkImageSubresourceRange subresourceRange);

	GpuObject GetImage() { return m_Image; }
	GpuObject GetView() { return m_View; }
	GpuObject GetSampler() { return m_Sampler; }

	unsigned int GetWidth() const { return (unsigned int)m_Width; }
	unsigned int GetHeight() const { return (unsigned int)m_Width; }

private:
	Graphics * m_pGraphics;
	GpuObject m_Image;
	GpuObject m_Memory;
	GpuObject m_View;
	GpuObject m_Sampler = VK_NULL_HANDLE;
	uint32 m_ImageLayout = 0;
	bool m_ImageOwned = true;
	int m_MemorySize = 0;

	int m_Width = 0;
	int m_Height = 0;
	int m_Components = 0;
};
