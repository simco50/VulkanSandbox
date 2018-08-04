#include "stdafx.h"
#include "CommandBuffer.h"
#include "Graphics.h"
#include "VertexBuffer.h"
#include "IndexBuffer.h"
#include "Texture2D.h"


CommandBuffer::CommandBuffer(Graphics* pGraphics) :
	m_pGraphics(pGraphics)
{
	VkCommandBufferAllocateInfo commandBufferAllocateInfo = {};
	commandBufferAllocateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	commandBufferAllocateInfo.commandBufferCount = 1;
	commandBufferAllocateInfo.commandPool = pGraphics->GetCommandPool();
	commandBufferAllocateInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	commandBufferAllocateInfo.pNext = nullptr;
	vkAllocateCommandBuffers(pGraphics->GetDevice(), &commandBufferAllocateInfo, &m_Buffer);
}

CommandBuffer::~CommandBuffer()
{
	vkFreeCommandBuffers(m_pGraphics->GetDevice(), m_pGraphics->GetCommandPool(), 1, &m_Buffer);
}

void CommandBuffer::Begin()
{
	VkCommandBufferBeginInfo beginInfo = {};
	beginInfo.flags = 0;
	beginInfo.pInheritanceInfo = nullptr;
	beginInfo.pNext = nullptr;
	beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;

	vkBeginCommandBuffer(m_Buffer, &beginInfo);
}

void CommandBuffer::End()
{
	vkEndCommandBuffer(m_Buffer);
}

void CommandBuffer::BeginRenderPass(VkFramebuffer frameBuffer, VkRenderPass renderPass, unsigned int width, unsigned int height)
{
	/* We cannot bind the vertex buffer until we begin a renderpass */
	VkClearValue clearValues[2];
	clearValues[0].color.float32[0] = 0.2f;
	clearValues[0].color.float32[1] = 0.2f;
	clearValues[0].color.float32[2] = 0.2f;
	clearValues[0].color.float32[3] = 0.2f;
	clearValues[1].depthStencil.depth = 1.0f;
	clearValues[1].depthStencil.stencil = 0;

	VkRenderPassBeginInfo rpBegin = {};
	rpBegin.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
	rpBegin.pNext = NULL;
	rpBegin.renderPass = renderPass;
	rpBegin.renderArea.offset.x = 0;
	rpBegin.renderArea.offset.y = 0;
	rpBegin.renderArea.extent.width = width;
	rpBegin.renderArea.extent.height = height;
	rpBegin.clearValueCount = 2;
	rpBegin.pClearValues = clearValues;
	rpBegin.framebuffer = frameBuffer;

	vkCmdBeginRenderPass(m_Buffer, &rpBegin, VK_SUBPASS_CONTENTS_INLINE);
}

void CommandBuffer::EndRenderPass()
{
	vkCmdEndRenderPass(m_Buffer);
}

void CommandBuffer::SetGraphicsPipeline(VkPipeline pipeline)
{
	vkCmdBindPipeline(m_Buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline);
}

void CommandBuffer::SetViewport(const VkViewport& viewport)
{
	vkCmdSetViewport(m_Buffer, 0, 1, &viewport);
}

void CommandBuffer::SetVertexBuffer(int index, VertexBuffer* pVertexBuffer)
{
	const VkDeviceSize offsets[1] = { 0 };
	vkCmdBindVertexBuffers(m_Buffer, index, 1, &pVertexBuffer->GetBuffer(), offsets);
}

void CommandBuffer::SetIndexBuffer(int index, IndexBuffer* pIndexBuffer)
{
	vkCmdBindIndexBuffer(m_Buffer, pIndexBuffer->GetBuffer(), 0, VK_INDEX_TYPE_UINT32);
}

void CommandBuffer::SetDescriptorSet(VkPipelineLayout pipelineLayout, int setIndex, VkDescriptorSet set, const std::vector<unsigned int>& dynamicOffsets)
{
	vkCmdBindDescriptorSets(m_Buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, setIndex, 1, &set, dynamicOffsets.size(), dynamicOffsets.data());
}

void CommandBuffer::Draw(unsigned int vertexCount, unsigned int vertexStart)
{
	vkCmdDraw(m_Buffer, vertexCount, 1, vertexStart, 0);
}

void CommandBuffer::DrawIndexed(unsigned int indexCount, unsigned int indexStart)
{
	vkCmdDrawIndexed(m_Buffer, indexCount, 1, indexStart, 0, 0);
}

void CommandBuffer::CopyBufferToImage(VkBuffer buffer, Texture2D* pImage)
{
	VkBufferImageCopy copyRegion = {};
	copyRegion.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	copyRegion.imageSubresource.baseArrayLayer = 0;
	copyRegion.imageSubresource.layerCount = 1;
	copyRegion.imageSubresource.mipLevel = 0;
	copyRegion.bufferOffset = 0;
	copyRegion.imageExtent.width = pImage->GetWidth();
	copyRegion.imageExtent.height = pImage->GetHeight();
	copyRegion.imageExtent.depth = 1;

	vkCmdCopyBufferToImage(m_Buffer, buffer, (VkImage)pImage->GetImage(), VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &copyRegion);
}

void CommandBuffer::CopyBuffer(VkBuffer source, VkBuffer target, int size)
{
	VkBufferCopy copyRegion = {};
	copyRegion.size = size;
	vkCmdCopyBuffer(m_Buffer, source, target, 1, &copyRegion);
}
