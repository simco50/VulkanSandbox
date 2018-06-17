#include "stdafx.h"
#include "IndexBuffer.h"
#include "Graphics.h"

IndexBuffer::IndexBuffer(Graphics* pGraphics) :
	m_pGraphics(pGraphics)
{

}

IndexBuffer::~IndexBuffer()
{
	vkFreeMemory(m_pGraphics->GetDevice(), m_Memory, nullptr);
	vkDestroyBuffer(m_pGraphics->GetDevice(), m_Buffer, nullptr);
}

void IndexBuffer::SetSize(const int count, bool smallIndices, const bool dynamic /*= false*/)
{
	UNREFERENCED_PARAMETER(dynamic);

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

	VkMemoryRequirements memoryRequirements;
	vkGetBufferMemoryRequirements(m_pGraphics->GetDevice(), m_Buffer, &memoryRequirements);
	m_BufferSize = (int)memoryRequirements.size;
	VkMemoryAllocateInfo memoryAllocateInfo = {};
	memoryAllocateInfo.allocationSize = memoryRequirements.size;
	m_pGraphics->MemoryTypeFromProperties(memoryRequirements.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, &memoryAllocateInfo.memoryTypeIndex);
	memoryAllocateInfo.pNext = nullptr;
	memoryAllocateInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	vkAllocateMemory(m_pGraphics->GetDevice(), &memoryAllocateInfo, nullptr, &m_Memory);
	vkBindBufferMemory(m_pGraphics->GetDevice(), m_Buffer, m_Memory, 0);
}

void IndexBuffer::SetData(void* pData)
{
	m_pGraphics->CopyBufferWithStaging(m_Buffer, pData);
}
