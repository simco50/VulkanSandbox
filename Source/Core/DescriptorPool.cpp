#include "stdafx.h"
#include "DescriptorPool.h"
#include "Graphics.h"


DescriptorPool::DescriptorPool(Graphics* pGraphics) :
	m_pGraphics(pGraphics)
{
	const VkPhysicalDeviceLimits& limits = pGraphics->GetDeviceProperties().limits;

	std::vector<VkDescriptorPoolSize> descriptorPoolSizes(2);
	descriptorPoolSizes[0].descriptorCount = limits.maxDescriptorSetUniformBuffersDynamic;
	descriptorPoolSizes[0].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC;
	descriptorPoolSizes[1].descriptorCount = limits.sampledImageColorSampleCounts;
	descriptorPoolSizes[1].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;

	VkDescriptorPoolCreateInfo descriptorPoolCreateInfo = {};
	descriptorPoolCreateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
	descriptorPoolCreateInfo.flags = 0;
	descriptorPoolCreateInfo.maxSets = 1048;
	descriptorPoolCreateInfo.pNext = nullptr;
	descriptorPoolCreateInfo.poolSizeCount = (uint32)descriptorPoolSizes.size();
	descriptorPoolCreateInfo.pPoolSizes = descriptorPoolSizes.data();
	vkCreateDescriptorPool(pGraphics->GetDevice(), &descriptorPoolCreateInfo, nullptr, &m_Pool);
}

DescriptorPool::~DescriptorPool()
{
	vkDestroyDescriptorPool(m_pGraphics->GetDevice(), m_Pool, nullptr);
}

VkDescriptorSet DescriptorPool::Allocate(VkDescriptorSetLayout layout)
{
	VkDescriptorSet set;
	VkDescriptorSetAllocateInfo descriptorAllocateInfo[1];
	descriptorAllocateInfo[0].sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
	descriptorAllocateInfo[0].descriptorPool = m_Pool;
	descriptorAllocateInfo[0].descriptorSetCount = 1;
	descriptorAllocateInfo[0].pNext = nullptr;
	descriptorAllocateInfo[0].pSetLayouts = &layout;
	vkAllocateDescriptorSets(m_pGraphics->GetDevice(), descriptorAllocateInfo, &set);
	return set;
}

void DescriptorPool::Free(const VkDescriptorSet& set)
{
	vkFreeDescriptorSets(m_pGraphics->GetDevice(), m_Pool, 1, &set);
}

void DescriptorPool::Free(const std::vector<VkDescriptorSet>& sets)
{
	vkFreeDescriptorSets(m_pGraphics->GetDevice(), m_Pool, (unsigned int)sets.size(), sets.data());
}
