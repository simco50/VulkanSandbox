#include "stdafx.h"
#include "VulkanAllocator.h"
#include "Graphics.h"

VulkanAllocator::VulkanAllocator(VkPhysicalDevice physicalDevice, VkDevice device) :
	m_Device(device)
{
	vkGetPhysicalDeviceMemoryProperties(physicalDevice, &m_DeviceMemoryProperties);
}

VulkanAllocator::~VulkanAllocator()
{
	for (auto& pair : m_MemoryPools)
	{
		if (pair.second.Memory != VK_NULL_HANDLE)
		{
			if (pair.second.CpuVisible)
			{
				vkUnmapMemory(m_Device, pair.second.Memory);
			}
			vkFreeMemory(m_Device, pair.second.Memory, nullptr);
		}
	}
}

bool VulkanAllocator::MemoryTypeFromProperties(uint32_t typeBits, VkFlags requirements_mask, uint32_t *typeIndex)
{
	// Search memtypes to find first index with those properties
	for (uint32_t i = 0; i <m_DeviceMemoryProperties.memoryTypeCount; i++)
	{
		if ((typeBits & 1) == 1)
		{
			// Type is available, does it match user properties?
			if ((m_DeviceMemoryProperties.memoryTypes[i].propertyFlags & requirements_mask) == requirements_mask)
			{
				*typeIndex = i;
				return true;
			}
		}
		typeBits >>= 1;
	}
	// No memory types matched, return failure
	return false;
}

VulkanAllocation VulkanAllocator::Allocate(VkImage image, bool cpuVisible)
{
	VkMemoryRequirements requirements;
	vkGetImageMemoryRequirements(m_Device, image, &requirements);

	return Allocate(requirements, cpuVisible);
}

VulkanAllocation VulkanAllocator::Allocate(VkBuffer buffer, bool cpuVisible)
{
	VkMemoryRequirements requirements;
	vkGetBufferMemoryRequirements(m_Device, buffer, &requirements);

	return Allocate(requirements, cpuVisible);
}

VulkanAllocation VulkanAllocator::Allocate(VkMemoryRequirements& requirements, bool cpuVisible)
{
	uint32 index = 0;
	MemoryTypeFromProperties(requirements.memoryTypeBits, cpuVisible ? VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT : VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, &index);

	MemoryPool& pool = m_MemoryPools[index];
	if (pool.Memory == VK_NULL_HANDLE)
	{
		pool.TypeBits = index;
		pool.CurrentOffset = 0;
		VkMemoryAllocateInfo allocateInfo = {};
		allocateInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
		allocateInfo.pNext = nullptr;
		allocateInfo.allocationSize = POOL_SIZE;
		allocateInfo.memoryTypeIndex = index;
		vkAllocateMemory(m_Device, &allocateInfo, nullptr, &pool.Memory);
		pool.CpuVisible = cpuVisible;
		if (pool.CpuVisible)
		{
			vkMapMemory(m_Device, pool.Memory, 0, POOL_SIZE, 0, &pool.pCpuPointer);
		}
	}
	VulkanAllocation allocation(pool, requirements.size);
	int desiredSize = (requirements.size + requirements.alignment - 1) & ~(requirements.alignment - 1);
	pool.CurrentOffset += desiredSize;
	++pool.Allocations;
	return allocation;
}

void VulkanAllocator::Free(VulkanAllocation& allocation)
{
	//Not implemented
}
