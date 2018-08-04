#include "stdafx.h"
#include "MemoryPool.h"
#include "Graphics.h"


MemoryPool::MemoryPool(Graphics* pGraphics) :
	m_pGraphics(pGraphics)
{
	
}

MemoryPool::~MemoryPool()
{
}

Allocation MemoryPool::Allocate(uint32 type, uint64 size)
{
	MemoryPage& page = AllocatePage(type, size);
	Allocation allocation;
	allocation.Memory = page.Memory;
	allocation.Offset = page.CurrentOffset;
	page.CurrentOffset += size;
}

void MemoryPool::Free(Allocation&& allocation)
{

}

MemoryPage& MemoryPool::AllocatePage(uint32 type, uint64 size)
{
	MemoryPage page;
	page.Size = size;
	page.CurrentOffset = 0;

	VkMemoryAllocateInfo memoryAllocateInfo = {};
	memoryAllocateInfo.allocationSize = size;
	m_pGraphics->MemoryTypeFromProperties(type, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, &memoryAllocateInfo.memoryTypeIndex);
	memoryAllocateInfo.pNext = nullptr;
	memoryAllocateInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	vkAllocateMemory(m_pGraphics->GetDevice(), &memoryAllocateInfo, nullptr, &page.Memory);

	m_Allocations[type].push_back(page);
	return m_Allocations.at(type).back();
}
