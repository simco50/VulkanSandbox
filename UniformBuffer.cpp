#include "stdafx.h"
#include "UniformBuffer.h"
#include "Graphics.h"

UniformBuffer::UniformBuffer(Graphics* pGraphics) :
	m_pGraphics(pGraphics)
{

}

UniformBuffer::~UniformBuffer()
{
	vkUnmapMemory(m_pGraphics->GetDevice(), m_Memory);
	vkFreeMemory(m_pGraphics->GetDevice(), m_Memory, nullptr);
	vkDestroyBuffer(m_pGraphics->GetDevice(), m_Buffer, nullptr);
}

void* UniformBuffer::Map()
{
	return m_pCurrentTarget;
}

void UniformBuffer::Unmap()
{
	m_pCurrentTarget = (char*)m_pCurrentTarget + m_BufferSize / m_pGraphics->GetBackbufferCount();
}

bool UniformBuffer::SetSize(const int size)
{
	int alignment = (int)m_pGraphics->GetDeviceProperties().limits.minUniformBufferOffsetAlignment;
	int desiredSize = size;
	desiredSize = (desiredSize + alignment - 1) & ~(alignment - 1);
	desiredSize *= m_pGraphics->GetBackbufferCount();

	m_BufferSize = desiredSize;

	VkBufferCreateInfo createInfo = {};
	createInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
	createInfo.flags = 0;
	createInfo.pNext = nullptr;
	createInfo.pQueueFamilyIndices = nullptr;
	createInfo.queueFamilyIndexCount = 0;
	createInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
	createInfo.size = desiredSize;
	createInfo.usage = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;
	if (vkCreateBuffer(m_pGraphics->GetDevice(), &createInfo, nullptr, &m_Buffer) != VK_SUCCESS)
	{
		return false;
	}

	VkMemoryRequirements memoryRequirements = {};
	vkGetBufferMemoryRequirements(m_pGraphics->GetDevice(), m_Buffer, &memoryRequirements);

	VkMemoryAllocateInfo allocateInfo = {};
	allocateInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	allocateInfo.pNext = nullptr;
	allocateInfo.allocationSize = (int)memoryRequirements.size;
	m_pGraphics->MemoryTypeFromProperties(memoryRequirements.memoryTypeBits, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, &allocateInfo.memoryTypeIndex);
	if (vkAllocateMemory(m_pGraphics->GetDevice(), &allocateInfo, nullptr, &m_Memory) != VK_SUCCESS)
	{
		return false;
	}
	if(vkBindBufferMemory(m_pGraphics->GetDevice(), m_Buffer, m_Memory, 0) != VK_SUCCESS)
	{
		return false;
	}

	if (vkMapMemory(m_pGraphics->GetDevice(), m_Memory, 0, m_BufferSize, 0, &m_pDataBegin) != VK_SUCCESS)
	{
		return false;
	}
	m_pCurrentTarget = m_pDataBegin;

	return true;
}

bool UniformBuffer::SetData(const int offset, const int size, void* pData)
{
	void* pTarget = Map();
	memcpy(pTarget, pData, size);
	Unmap();
	return true;
}

int UniformBuffer::GetOffset(int frameIndex) const
{
	return frameIndex * m_BufferSize / m_pGraphics->GetBackbufferCount();
}

void UniformBuffer::Flush()
{
	m_pCurrentTarget = m_pDataBegin;
}
