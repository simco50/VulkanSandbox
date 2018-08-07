#include "stdafx.h"
#include "Texture2D.h"
#include "Core/Graphics.h"

#define STB_IMAGE_IMPLEMENTATION
#include "External/stb_image.h"
#include "Core/VulkanAllocator.h"


Texture2D::Texture2D(Graphics* pGraphics) :
	m_pGraphics(pGraphics)
{

}

Texture2D::~Texture2D()
{
	if (m_ImageOwned)
	{
		vkDestroyImage(m_pGraphics->GetDevice(), (VkImage)m_Image, nullptr);
	}
	vkDestroyImageView(m_pGraphics->GetDevice(), (VkImageView)m_View, nullptr);

	if (m_Sampler != VK_NULL_HANDLE)
	{
		vkDestroySampler(m_pGraphics->GetDevice(), (VkSampler)m_Sampler, nullptr);
	}
}

bool Texture2D::Load(const std::string& filePath)
{
	std::ifstream stream(filePath, std::ios::ate | std::ios::binary);
	std::vector<char> buffer((size_t)stream.tellg());
	stream.seekg(0);
	stream.read(buffer.data(), buffer.size());

	unsigned char* pPixels = stbi_load_from_memory((unsigned char*)buffer.data(), (uint32)buffer.size(), &m_Width, &m_Height, &m_Components, 4);
	SetSize(m_Width, m_Height, VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT, 1, 0);
	SetData(1, 0, 0, m_Width, m_Height, pPixels);
	stbi_image_free(pPixels);

	UpdateParameters();

	return true;
}

void Texture2D::SetSize(const int width, const int height, const unsigned int format, unsigned int usage, const int multiSample, int64 pTexture)
{
	if (pTexture == VK_NULL_HANDLE)
	{
		m_ImageOwned = true;
		VkImageCreateInfo imageCreateInfo = {};
		imageCreateInfo.arrayLayers = 1;
		imageCreateInfo.extent.depth = 1;
		imageCreateInfo.extent.width = width;
		imageCreateInfo.extent.height = height;
		imageCreateInfo.flags = 0;
		imageCreateInfo.format = (VkFormat)format;
		imageCreateInfo.imageType = VK_IMAGE_TYPE_2D;
		imageCreateInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		imageCreateInfo.mipLevels = 1;
		imageCreateInfo.pNext = nullptr;
		imageCreateInfo.pQueueFamilyIndices = nullptr;
		imageCreateInfo.queueFamilyIndexCount = 0;
		imageCreateInfo.samples = (VkSampleCountFlagBits)multiSample;
		imageCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
		imageCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
		imageCreateInfo.usage = usage;
		imageCreateInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
		vkCreateImage(m_pGraphics->GetDevice(), &imageCreateInfo, nullptr, (VkImage*)&m_Image);
		m_ImageLayout = VK_IMAGE_LAYOUT_UNDEFINED;

		VulkanAllocation allocation = m_pGraphics->GetAllocator()->Allocate((VkImage)m_Image, false);
		m_MemorySize = allocation.Size;
		vkBindImageMemory(m_pGraphics->GetDevice(), (VkImage)m_Image, allocation.Memory, allocation.Offset);
	}
	else
	{
		m_ImageOwned = false;
		m_Image = pTexture;
	}

	//Create Image view
	VkImageViewCreateInfo viewCreateInfo;
	viewCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
	viewCreateInfo.components.r = VK_COMPONENT_SWIZZLE_R;
	viewCreateInfo.components.g = VK_COMPONENT_SWIZZLE_G;
	viewCreateInfo.components.b = VK_COMPONENT_SWIZZLE_B;
	viewCreateInfo.components.a = VK_COMPONENT_SWIZZLE_A;
	viewCreateInfo.flags = 0;
	viewCreateInfo.format = (VkFormat)format;
	viewCreateInfo.image = (VkImage)m_Image;
	viewCreateInfo.pNext = nullptr;
	if (usage & VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT || usage & VK_IMAGE_USAGE_SAMPLED_BIT)
	{
		viewCreateInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	}
	else if (usage == VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT)
	{
		viewCreateInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
	}
	viewCreateInfo.subresourceRange.baseArrayLayer = 0;
	viewCreateInfo.subresourceRange.baseMipLevel = 0;
	viewCreateInfo.subresourceRange.levelCount = 1;
	viewCreateInfo.subresourceRange.layerCount = 1;
	viewCreateInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
	vkCreateImageView(m_pGraphics->GetDevice(), &viewCreateInfo, nullptr, (VkImageView*)&m_View);
}

bool Texture2D::SetData(const unsigned int mipLevel, int x, int y, int width, int height, const void* pData)
{
	VkBuffer stagingBuffer;
	VkBufferCreateInfo createInfo = {};
	createInfo.flags = 0;
	createInfo.pNext = nullptr;
	createInfo.pQueueFamilyIndices = nullptr;
	createInfo.queueFamilyIndexCount = 0;
	createInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
	createInfo.size = m_MemorySize;
	createInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
	createInfo.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
	vkCreateBuffer(m_pGraphics->GetDevice(), &createInfo, nullptr, &stagingBuffer);

	VulkanAllocation allocation = m_pGraphics->GetAllocator()->Allocate(stagingBuffer, true);

	vkBindBufferMemory(m_pGraphics->GetDevice(), stagingBuffer, allocation.Memory, allocation.Offset);
	memcpy(allocation.pCpuPointer, pData, m_MemorySize);

	std::unique_ptr<CommandBuffer> copyCmd = m_pGraphics->GetTempCommandBuffer(true);
	
	VkImageSubresourceRange subresourceRange = {};
	subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	subresourceRange.baseMipLevel = 0;
	subresourceRange.levelCount = 1;
	subresourceRange.layerCount = 1;

	SetLayout(copyCmd->GetBuffer(), VK_IMAGE_ASPECT_COLOR_BIT, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, subresourceRange);
	copyCmd->CopyBufferToImage(stagingBuffer, this);
	SetLayout(copyCmd->GetBuffer(), VK_IMAGE_ASPECT_COLOR_BIT, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, subresourceRange);

	m_pGraphics->FlushCommandBuffer(copyCmd);

	m_pGraphics->GetAllocator()->Free(allocation);
	vkDestroyBuffer(m_pGraphics->GetDevice(), stagingBuffer, nullptr);

	return true;
}

void Texture2D::UpdateParameters()
{
	if (m_Sampler != VK_NULL_HANDLE)
	{
		vkDestroySampler(m_pGraphics->GetDevice(), (VkSampler)m_Sampler, nullptr);
	}

	VkSamplerCreateInfo samplerCreateInfo = {};
	samplerCreateInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
	samplerCreateInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
	samplerCreateInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
	samplerCreateInfo.anisotropyEnable = false;
	samplerCreateInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
	samplerCreateInfo.compareEnable = false;
	samplerCreateInfo.compareOp = VK_COMPARE_OP_NEVER;
	samplerCreateInfo.flags = 0;
	samplerCreateInfo.magFilter = VK_FILTER_LINEAR;
	samplerCreateInfo.maxAnisotropy = 1.0f;
	samplerCreateInfo.maxLod = 1.0f;
	samplerCreateInfo.minFilter = VK_FILTER_LINEAR;
	samplerCreateInfo.minLod = 0.0f;
	samplerCreateInfo.mipLodBias = 0;
	samplerCreateInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
	samplerCreateInfo.pNext = nullptr;
	samplerCreateInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
	samplerCreateInfo.unnormalizedCoordinates = VK_FALSE;
	vkCreateSampler(m_pGraphics->GetDevice(), &samplerCreateInfo, nullptr, (VkSampler*)&m_Sampler);
}

void Texture2D::SetLayout(VkCommandBuffer cmdBuffer, VkImageAspectFlags aspectMask, VkImageLayout newImageLayout, VkImageSubresourceRange subresourceRange)
{
	// Create an image barrier object
	VkImageMemoryBarrier imageMemoryBarrier = {};
	imageMemoryBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
	imageMemoryBarrier.pNext = nullptr;
	imageMemoryBarrier.oldLayout = (VkImageLayout)m_ImageLayout;
	imageMemoryBarrier.newLayout = newImageLayout;
	imageMemoryBarrier.image = (VkImage)m_Image;
	imageMemoryBarrier.subresourceRange = subresourceRange;

	// Only sets masks for layouts used in this example
	// For a more complete version that can be used with other layouts see vks::tools::setImageLayout

	// Source layouts (old)
	switch ((VkImageLayout)m_ImageLayout)
	{
	case VK_IMAGE_LAYOUT_UNDEFINED:
		// Only valid as initial layout, memory contents are not preserved
		// Can be accessed directly, no source dependency required
		imageMemoryBarrier.srcAccessMask = 0;
		break;
	case VK_IMAGE_LAYOUT_PREINITIALIZED:
		// Only valid as initial layout for linear images, preserves memory contents
		// Make sure host writes to the image have been finished
		imageMemoryBarrier.srcAccessMask = VK_ACCESS_HOST_WRITE_BIT;
		break;
	case VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL:
		// Old layout is transfer destination
		// Make sure any writes to the image have been finished
		imageMemoryBarrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
		break;
	}

	// Target layouts (new)
	switch (newImageLayout)
	{
	case VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL:
		// Transfer source (copy, blit)
		// Make sure any reads from the image have been finished
		imageMemoryBarrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
		break;
	case VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL:
		// Transfer destination (copy, blit)
		// Make sure any writes to the image have been finished
		imageMemoryBarrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
		break;
	case VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL:
		// Shader read (sampler, input attachment)
		imageMemoryBarrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
		break;
	}

	// Put barrier on top of pipeline
	VkPipelineStageFlags srcStageFlags = VK_PIPELINE_STAGE_ALL_COMMANDS_BIT;
	VkPipelineStageFlags destStageFlags = VK_PIPELINE_STAGE_ALL_COMMANDS_BIT;

	// Put barrier inside setup command buffer
	vkCmdPipelineBarrier(
		cmdBuffer,
		srcStageFlags,
		destStageFlags,
		0,
		0, nullptr,
		0, nullptr,
		1, &imageMemoryBarrier);

	m_ImageLayout = newImageLayout;
}
