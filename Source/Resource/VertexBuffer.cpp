#include "stdafx.h"
#include "VertexBuffer.h"
#include "Core/Graphics.h"
#include "Core/VulkanAllocator.h"

VertexBuffer::VertexBuffer(Graphics* pGraphics) :
	m_pGraphics(pGraphics)
{

}

VertexBuffer::~VertexBuffer()
{
	vkDestroyBuffer(m_pGraphics->GetDevice(), m_Buffer, nullptr);
}

void VertexBuffer::SetSize(const int size, const bool dynamic /*= false*/)
{
	m_Size = size;

	VkBufferCreateInfo createInfo = {};
	createInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
	createInfo.flags = 0;
	createInfo.pNext = nullptr;
	createInfo.pQueueFamilyIndices = nullptr;
	createInfo.queueFamilyIndexCount = 0;
	createInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
	createInfo.size = size;
	createInfo.usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT;
	vkCreateBuffer(m_pGraphics->GetDevice(), &createInfo, nullptr, &m_Buffer);

	VulkanAllocation allocation = m_pGraphics->GetAllocator()->Allocate(m_Buffer, dynamic);
	vkBindBufferMemory(m_pGraphics->GetDevice(), m_Buffer, allocation.Memory, allocation.Offset);
}

void VertexBuffer::SetData(const int size, const int offset, void* pData)
{
	m_pGraphics->CopyBufferWithStaging(m_Buffer, pData);
}
