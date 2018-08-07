#include "stdafx.h"
#include "IndexBuffer.h"
#include "Core/Graphics.h"
#include "Core/VulkanAllocator.h"

IndexBuffer::IndexBuffer(Graphics* pGraphics) :
	m_pGraphics(pGraphics)
{

}

IndexBuffer::~IndexBuffer()
{
	vkDestroyBuffer(m_pGraphics->GetDevice(), m_Buffer, nullptr);
}

void IndexBuffer::SetSize(const int count, bool smallIndices, const bool dynamic /*= false*/)
{
	m_IndexSize = smallIndices ? sizeof(unsigned short) : sizeof(unsigned int);
	m_IndexCount = count;
	m_Size = count * m_IndexSize;

	VkBufferCreateInfo createInfo = {};
	createInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
	createInfo.flags = 0;
	createInfo.pNext = nullptr;
	createInfo.pQueueFamilyIndices = nullptr;
	createInfo.queueFamilyIndexCount = 0;
	createInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
	createInfo.size = m_Size;
	createInfo.usage = VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT;
	vkCreateBuffer(m_pGraphics->GetDevice(), &createInfo, nullptr, &m_Buffer);

	VulkanAllocation allocation = m_pGraphics->GetAllocator()->Allocate(m_Buffer, dynamic);

	vkBindBufferMemory(m_pGraphics->GetDevice(), m_Buffer, allocation.Memory, allocation.Offset);
}

void IndexBuffer::SetData(void* pData)
{
	m_pGraphics->CopyBufferWithStaging(m_Buffer, pData);
}
