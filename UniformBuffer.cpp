#include "stdafx.h"
#include "UniformBuffer.h"
#include "Graphics.h"

UniformBuffer::UniformBuffer(Graphics* pGraphics) :
	m_pGraphics(pGraphics)
{

}

UniformBuffer::~UniformBuffer()
{
	vkFreeMemory(m_pGraphics->GetDevice(), m_Memory, nullptr);
	vkDestroyBuffer(m_pGraphics->GetDevice(), m_Buffer, nullptr);
}

bool UniformBuffer::SetSize(const int size)
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
	createInfo.usage = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;
	if (vkCreateBuffer(m_pGraphics->GetDevice(), &createInfo, nullptr, &m_Buffer) != VK_SUCCESS)
	{
		return false;
	}

	VkMemoryRequirements memoryRequirements = {};
	vkGetBufferMemoryRequirements(m_pGraphics->GetDevice(), m_Buffer, &memoryRequirements);
	m_BufferSize = (int)memoryRequirements.size;
	VkMemoryAllocateInfo allocateInfo = {};
	allocateInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	allocateInfo.pNext = nullptr;
	allocateInfo.allocationSize = memoryRequirements.size;
	m_pGraphics->MemoryTypeFromProperties(memoryRequirements.memoryTypeBits, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, &allocateInfo.memoryTypeIndex);
	if (vkAllocateMemory(m_pGraphics->GetDevice(), &allocateInfo, nullptr, &m_Memory) != VK_SUCCESS)
	{
		return false;
	}
	if(vkBindBufferMemory(m_pGraphics->GetDevice(), m_Buffer, m_Memory, 0) != VK_SUCCESS)
	{
		return false;
	}
	return true;
}

bool UniformBuffer::SetData(const int offset, const int size, void* pData)
{
	void* pTarget = nullptr;
	if (vkMapMemory(m_pGraphics->GetDevice(), m_Memory, offset, m_BufferSize, 0, &pTarget) != VK_SUCCESS)
	{
		return false;
	}
	memcpy((char*)pTarget + offset, (char*)pData + offset, size);
	vkUnmapMemory(m_pGraphics->GetDevice(), m_Memory);
	return true;
}