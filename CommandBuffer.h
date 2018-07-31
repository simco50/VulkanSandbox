#pragma once
class Graphics;
class IndexBuffer;
class VertexBuffer;
class Texture2D;

class CommandBuffer
{
public:
	CommandBuffer(Graphics* pGraphics);
	~CommandBuffer();

	void Begin();
	void End();

	void BeginRenderPass(VkFramebuffer frameBuffer, VkRenderPass renderPass, unsigned int width, unsigned int height);
	void EndRenderPass();

	void SetGraphicsPipeline(VkPipeline pipeline);
	void SetViewport(const VkViewport& viewport);
	void SetVertexBuffer(int index, VertexBuffer* pVertexBuffer);
	void SetIndexBuffer(int index, IndexBuffer* pIndexBuffer);
	void SetDescriptorSets(VkPipelineLayout pipelineLayout, const std::vector<VkDescriptorSet>& sets, const std::vector<unsigned int>& dynamicOffsets);
	void Draw(unsigned int vertexCount, unsigned int vertexStart);
	void DrawIndexed(unsigned int indexCount, unsigned int indexStart);

	void CopyImageToBuffer(VkBuffer buffer, Texture2D* pImage);
	void CopyBuffer(VkBuffer source, VkBuffer target, int size);

	VkCommandBuffer GetBuffer() const { return m_Buffer; }

private:
	Graphics * m_pGraphics;
	VkCommandBuffer m_Buffer;
};

