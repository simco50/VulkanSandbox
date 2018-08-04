#pragma once
class Graphics;

struct MemoryPool
{
	int TypeBits = 0;
	VkDeviceMemory Memory = VK_NULL_HANDLE;
	int CurrentOffset = 0;
	bool CpuVisible = false;
	void* pCpuPointer = nullptr;
	int Allocations = 0;
};

struct VulkanAllocation
{
	VulkanAllocation(MemoryPool& pool, int size) :
		Memory(pool.Memory), 
		Offset(pool.CurrentOffset),
		Size(size),
		pCpuPointer((char*)pool.pCpuPointer + pool.CurrentOffset),
		pParentPool(&pool)
	{}
	VkDeviceMemory Memory;
	int Offset;
	int Size;
	void* pCpuPointer;

	MemoryPool* pParentPool;

	void Free()
	{
		assert(pParentPool->CurrentOffset == Offset + Size);
		
		pParentPool->CurrentOffset -= Size;
		--pParentPool->Allocations;

		Memory = VK_NULL_HANDLE;
		Offset = -1;
		Size = -1;
		pCpuPointer = nullptr;
	}
};

class VulkanAllocator
{
public:
	VulkanAllocator(VkPhysicalDevice physicalDevice, VkDevice device);
	~VulkanAllocator();

	VulkanAllocation Allocate(VkImage image, bool cpuVisible);
	VulkanAllocation Allocate(VkBuffer buffer, bool cpuVisible);
	void Free(VulkanAllocation& allocation);

private:
	bool MemoryTypeFromProperties(uint32_t typeBits, VkFlags requirements_mask, uint32_t *typeIndex);
	VulkanAllocation Allocate(VkMemoryRequirements& requirements, bool cpuVisible);

	static const int POOL_SIZE = 4096 * 40960;

	VkDevice m_Device;
	std::map<uint32, MemoryPool> m_MemoryPools;
	VkPhysicalDeviceMemoryProperties m_DeviceMemoryProperties;
};

