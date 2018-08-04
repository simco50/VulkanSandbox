#pragma once
#include <map>
class Graphics;

struct Allocation
{
	VkDeviceMemory Memory;
	uint64 Offset;
};

struct MemoryPage
{
	VkDeviceMemory Memory;
	uint64 Size;
	uint64 CurrentOffset;
};

class MemoryPool
{
public:
	MemoryPool(Graphics* pGraphics);
	~MemoryPool();

	Allocation Allocate(uint32 type, uint64 size);
	void Free(Allocation&& allocation);

private:
	Graphics * m_pGraphics;
	MemoryPage& AllocatePage(uint32 type, uint64 size);

	std::map<uint64, std::vector<MemoryPage>> m_Allocations;
};

