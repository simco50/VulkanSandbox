#include "stdafx.h"
#include "VertexBuffer.h"
#include "Graphics.h"

VertexBuffer::VertexBuffer(Graphics* pGraphics) :
	m_pGraphics(pGraphics)
{

}

VertexBuffer::~VertexBuffer()
{
	vkFreeMemory(m_pGraphics->GetDevice(), m_Memory, nullptr);
	vkDestroyBuffer(m_pGraphics->GetDevice(), m_Buffer, nullptr);
}

void VertexBuffer::SetSize(const int size, const bool dynamic /*= false*/)
{
	UNREFERENCED_PARAMETER(dynamic);

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

void VertexBuffer::SetData(const int size, const int offset, void* pData)
{
	m_pGraphics->CopyBufferWithStaging(m_Buffer, pData);
}
