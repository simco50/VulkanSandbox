#include "stdafx.h"
#include "Graphics.h"

#include <SDL_syswm.h>
#include "Shader.h"
#include "UniformBuffer.h"
#include "VertexBuffer.h"
#include "IndexBuffer.h"
#include "Texture2D.h"
#include "Cube.h"
#include "Drawable.h"
#include "Material.h"
#include "DescriptorPool.h"

Graphics::Graphics()
{
}

Graphics::~Graphics()
{
}

HWND Graphics::GetWindow() const
{
	SDL_SysWMinfo sysInfo;
	SDL_VERSION(&sysInfo.version);
	SDL_GetWindowWMInfo(m_pWindow, &sysInfo);
	return sysInfo.info.win.window;
}

bool Graphics::MemoryTypeFromProperties(uint32_t typeBits, VkFlags requirements_mask, uint32_t *typeIndex) 
{
	// Search memtypes to find first index with those properties
	for (uint32_t i = 0; i < m_MemoryProperties.memoryTypeCount; i++) 
	{
		if ((typeBits & 1) == 1) 
		{
			// Type is available, does it match user properties?
			if ((m_MemoryProperties.memoryTypes[i].propertyFlags & requirements_mask) == requirements_mask) 
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

bool VulkanTranslateError(VkResult errorCode)
{
	std::string error;
	switch (errorCode)
	{
#define STR(r) case VK_ ##r: error = #r; break
		STR(NOT_READY);
		STR(TIMEOUT);
		STR(EVENT_SET);
		STR(EVENT_RESET);
		STR(INCOMPLETE);
		STR(ERROR_OUT_OF_HOST_MEMORY);
		STR(ERROR_OUT_OF_DEVICE_MEMORY);
		STR(ERROR_INITIALIZATION_FAILED);
		STR(ERROR_DEVICE_LOST);
		STR(ERROR_MEMORY_MAP_FAILED);
		STR(ERROR_LAYER_NOT_PRESENT);
		STR(ERROR_EXTENSION_NOT_PRESENT);
		STR(ERROR_FEATURE_NOT_PRESENT);
		STR(ERROR_INCOMPATIBLE_DRIVER);
		STR(ERROR_TOO_MANY_OBJECTS);
		STR(ERROR_FORMAT_NOT_SUPPORTED);
		STR(ERROR_SURFACE_LOST_KHR);
		STR(ERROR_NATIVE_WINDOW_IN_USE_KHR);
		STR(SUBOPTIMAL_KHR);
		STR(ERROR_OUT_OF_DATE_KHR);
		STR(ERROR_INCOMPATIBLE_DISPLAY_KHR);
		STR(ERROR_VALIDATION_FAILED_EXT);
		STR(ERROR_INVALID_SHADER_NV);
#undef STR
	default:
		error = "UNKNOWN_ERROR";
	}
	bool isError = errorCode != VK_SUCCESS;
	if (isError)
	{
		std::cout << error << std::endl;
	}
	return isError;
}

void Graphics::Initialize()
{
	//General initialization
	ConstructWindow();
	CreateVulkanInstance();
	CreateDevice();
	CreateSwapchain();
	CreateCommandPool();
	CreateCommandBuffers();
	CreateSynchronizationPrimitives();
	CreateDepthStencil();
	CreateRenderPass();
	CreatePipelineCache();
	CreateDescriptorPool();
	CreateFrameBuffer();

	m_pMaterial = std::make_unique<Material>(this);
	m_pMaterial->Load("Resources/Materials/Default.xml");

#pragma endregion

	std::unique_ptr<Cube> pCube = std::make_unique<Cube>(this);
	pCube->Create(1, 1, 1);
	pCube->SetPosition(-1.0f, 0.0f, 1.0f);
	m_Drawables.push_back(std::move(pCube));

	pCube = std::make_unique<Cube>(this);
	pCube->Create(1, 0.5f, 1);
	pCube->SetPosition(2.0f, 0.0f, 1.0f);
	m_Drawables.push_back(std::move(pCube));

	m_pImageTexture = new Texture2D(this);
	m_pImageTexture->Load("Resources/Textures/spot.png");

	VkDescriptorImageInfo texInfo;
	texInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
	texInfo.imageView = (VkImageView)m_pImageTexture->GetView();
	texInfo.sampler = (VkSampler)m_pImageTexture->GetSampler();

	//Create uniform buffer
	struct ModelBuffer
	{
		glm::mat4 ModelMatrix;
		glm::mat4 MvpMatrix;
	} ModelBufferData;

	m_ProjectionMatrix = glm::perspective(glm::radians(45.0f), 1.0f, 0.1f, 100.0f);
	m_ViewMatrix = glm::lookAt(
		glm::vec3(-5, 3, -10), // Camera is at (-5,3,-10), in World Space
		glm::vec3(0, 0, 0),    // and looks at the origin
		glm::vec3(0, -1, 0)    // Head is up (set to 0,-1,0 to look upside-down)
	);
	m_ModelMatrix = glm::mat4(1.0f);
	m_MvpMatrix = m_ProjectionMatrix * m_ViewMatrix * m_ModelMatrix;

	ModelBufferData.MvpMatrix = m_MvpMatrix;
	ModelBufferData.ModelMatrix = m_ModelMatrix;

	m_pUniformBuffer = new UniformBuffer(this);
	m_pUniformBuffer->SetSize(sizeof(ModelBuffer));
	m_pUniformBuffer->SetData(0, sizeof(ModelBuffer), &ModelBufferData);

	VkDescriptorBufferInfo ubInfo = {};
	ubInfo.buffer = m_pUniformBuffer->GetBuffer();
	ubInfo.range = VK_WHOLE_SIZE;
	ubInfo.offset = 0;

	std::vector<VkWriteDescriptorSet> writes(2);
	writes[0] = {};
	writes[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	writes[0].descriptorCount = 1;
	writes[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC;
	writes[0].dstBinding = 0;
	writes[0].dstSet = m_pMaterial->GetDescriptorSet(0);
	writes[0].pBufferInfo = &ubInfo;
	writes[0].pNext = nullptr;
	writes[0].dstArrayElement = 0;

	writes[1] = {};
	writes[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	writes[1].descriptorCount = 1;
	writes[1].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	writes[1].dstBinding = 1;
	writes[1].dstSet = m_pMaterial->GetDescriptorSet(0);
	writes[1].pImageInfo = &texInfo;
	writes[1].pNext = nullptr;
	writes[1].dstArrayElement = 0;
	vkUpdateDescriptorSets(m_Device, (uint32)writes.size(), writes.data(), 0, nullptr);

	BuildCommandBuffers();

	Gameloop();
}

void Graphics::CreateFrameBuffer()
{
	//Frame buffers
	VkImageView frameAttachments[2];
	frameAttachments[1] = (VkImageView)m_pDepthTexture->GetView();
	VkFramebufferCreateInfo frameBufferCreateInfo = {};
	frameBufferCreateInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
	frameBufferCreateInfo.attachmentCount = 2;
	frameBufferCreateInfo.flags = 0;
	frameBufferCreateInfo.width = m_WindowWidth;
	frameBufferCreateInfo.height = m_WindowHeight;
	frameBufferCreateInfo.layers = 1;
	frameBufferCreateInfo.pAttachments = frameAttachments;
	frameBufferCreateInfo.pNext = nullptr;
	frameBufferCreateInfo.renderPass = m_RenderPass;
	m_FrameBuffers.resize(m_SwapchainImages.size());
	for (size_t i = 0; i < m_SwapchainImages.size(); i++)
	{
		frameAttachments[0] = (VkImageView)m_SwapchainImages[i]->GetView();
		VulkanTranslateError(vkCreateFramebuffer(m_Device, &frameBufferCreateInfo, nullptr, &m_FrameBuffers[i]));
	}
}

void Graphics::CopyBufferWithStaging(VkBuffer targetBuffer, void* pData)
{
	VkMemoryRequirements targetBufferRequirements;
	vkGetBufferMemoryRequirements(m_Device, targetBuffer, &targetBufferRequirements);

	struct StagingBuffer
	{
		VkDeviceMemory Memory;
		VkBuffer Buffer;
	} stagingBuffer;

	VkBufferCreateInfo bufferCreateInfo = {};
	bufferCreateInfo.flags = 0;
	bufferCreateInfo.pNext = nullptr;
	bufferCreateInfo.pQueueFamilyIndices = nullptr;
	bufferCreateInfo.queueFamilyIndexCount = 0;
	bufferCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
	bufferCreateInfo.size = targetBufferRequirements.size;
	bufferCreateInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
	bufferCreateInfo.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
	vkCreateBuffer(m_Device, &bufferCreateInfo, nullptr, &stagingBuffer.Buffer);

	VkMemoryRequirements memoryRequirements = {};
	vkGetBufferMemoryRequirements(m_Device, stagingBuffer.Buffer, &memoryRequirements);

	VkMemoryAllocateInfo allocationInfo = {};
	allocationInfo.pNext = nullptr;
	allocationInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	allocationInfo.allocationSize = memoryRequirements.size;
	MemoryTypeFromProperties(memoryRequirements.memoryTypeBits, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, &allocationInfo.memoryTypeIndex);
	vkAllocateMemory(m_Device, &allocationInfo, nullptr, &stagingBuffer.Memory);
	vkBindBufferMemory(m_Device, stagingBuffer.Buffer, stagingBuffer.Memory, 0);

	void* pTarget = nullptr;
	vkMapMemory(m_Device, stagingBuffer.Memory, 0, memoryRequirements.size, 0, &pTarget);
	memcpy(pTarget, pData, (size_t)memoryRequirements.size);
	vkUnmapMemory(m_Device, stagingBuffer.Memory);

	VkBufferCopy copyRegion = {};
	copyRegion.size = memoryRequirements.size;

	std::unique_ptr<CommandBuffer> copyCmd = GetTempCommandBuffer(true);
	copyCmd->CopyBuffer(stagingBuffer.Buffer, targetBuffer, (int)memoryRequirements.size);
	FlushCommandBuffer(copyCmd);

	vkDestroyBuffer(m_Device, stagingBuffer.Buffer, nullptr);
	vkFreeMemory(m_Device, stagingBuffer.Memory, nullptr);
}

std::unique_ptr<CommandBuffer> Graphics::GetTempCommandBuffer(const bool begin)
{
	std::unique_ptr<CommandBuffer> pBuffer = std::make_unique<CommandBuffer>(this);
	if (begin)
	{
		pBuffer->Begin();
	}
	return pBuffer;
}

void Graphics::FlushCommandBuffer(std::unique_ptr<CommandBuffer>& cmdBuffer)
{
	VkCommandBuffer buffer = cmdBuffer->GetBuffer();

	vkEndCommandBuffer(buffer);
	VkSubmitInfo submitInfo = {};
	submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	submitInfo.pNext = nullptr;
	submitInfo.commandBufferCount = 1;
	submitInfo.pCommandBuffers = &buffer;
	submitInfo.pSignalSemaphores = nullptr;
	submitInfo.pWaitDstStageMask = nullptr;
	submitInfo.pWaitSemaphores = nullptr;
	submitInfo.signalSemaphoreCount = 0;
	submitInfo.waitSemaphoreCount = 0;
	
	VkFenceCreateInfo fenceCreateInfo = {};
	fenceCreateInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
	fenceCreateInfo.flags = 0;
	fenceCreateInfo.pNext = nullptr;
	VkFence fence;
	vkCreateFence(m_Device, &fenceCreateInfo, nullptr, &fence);
	vkQueueSubmit(m_DeviceQueue, 1, &submitInfo, fence);
	vkWaitForFences(m_Device, 1, &fence, VK_TRUE, 1000000000);

	vkDestroyFence(m_Device, fence, nullptr);

	cmdBuffer.reset();
}

void Graphics::CreatePipelineCache()
{
	VkPipelineCacheCreateInfo cacheCreateInfo = {};
	cacheCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_CACHE_CREATE_INFO;
	cacheCreateInfo.pNext = nullptr;
	cacheCreateInfo.flags = 0;
	cacheCreateInfo.initialDataSize = 0;
	cacheCreateInfo.pInitialData = nullptr;
	vkCreatePipelineCache(m_Device, &cacheCreateInfo, nullptr, &m_PipelineCache);
}

void Graphics::CreateDescriptorPool()
{
	m_pDescriptorPool = std::make_unique<DescriptorPool>(this);
}

void Graphics::CreateRenderPass()
{
	//Create render pass
	//Attachments
	VkAttachmentDescription attachments[2];
	attachments[0].finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
	attachments[0].flags = 0;
	attachments[0].format = VK_FORMAT_B8G8R8A8_UNORM;
	attachments[0].initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	attachments[0].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
	attachments[0].samples = VK_SAMPLE_COUNT_1_BIT;
	attachments[0].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	attachments[0].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	attachments[0].storeOp = VK_ATTACHMENT_STORE_OP_STORE;

	attachments[1].finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
	attachments[1].flags = 0;
	attachments[1].format = VK_FORMAT_D16_UNORM;
	attachments[1].initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	attachments[1].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
	attachments[1].samples = VK_SAMPLE_COUNT_1_BIT;
	attachments[1].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	attachments[1].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	attachments[1].storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;

	//Subpass
	VkAttachmentReference colorReference = {};
	colorReference.attachment = 0;
	colorReference.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

	VkAttachmentReference depthReference = {};
	depthReference.attachment = 1;
	depthReference.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

	VkSubpassDescription subPassDescription = {};
	subPassDescription.colorAttachmentCount = 1;
	subPassDescription.flags = 0;
	subPassDescription.inputAttachmentCount = 0;
	subPassDescription.pColorAttachments = &colorReference;
	subPassDescription.pDepthStencilAttachment = &depthReference;
	subPassDescription.pInputAttachments = nullptr;
	subPassDescription.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
	subPassDescription.pPreserveAttachments = nullptr;
	subPassDescription.preserveAttachmentCount = 0;
	subPassDescription.pResolveAttachments = nullptr;

	//Render pass
	VkRenderPassCreateInfo renderPassInfo = {};
	renderPassInfo.attachmentCount = 2;
	renderPassInfo.dependencyCount = 0;
	renderPassInfo.flags = 0;
	renderPassInfo.pAttachments = attachments;
	renderPassInfo.pDependencies = nullptr;
	renderPassInfo.pNext = nullptr;
	renderPassInfo.pSubpasses = &subPassDescription;
	renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
	renderPassInfo.subpassCount = 1;
	VulkanTranslateError(vkCreateRenderPass(m_Device, &renderPassInfo, nullptr, &m_RenderPass));
}

void Graphics::CreateDepthStencil()
{
	m_pDepthTexture = new Texture2D(this);
	m_pDepthTexture->SetSize(m_WindowWidth, m_WindowHeight, VK_FORMAT_D16_UNORM, VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT, 1, 0);
}

void Graphics::CreateSynchronizationPrimitives()
{
	VkSemaphoreCreateInfo semaphoreCreateInfo = {};
	semaphoreCreateInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
	semaphoreCreateInfo.flags = 0;
	semaphoreCreateInfo.pNext = nullptr;
	vkCreateSemaphore(m_Device, &semaphoreCreateInfo, nullptr, &m_PresentCompleteSemaphore);
	vkCreateSemaphore(m_Device, &semaphoreCreateInfo, nullptr, &m_RenderCompleteSemaphore);

	VkFenceCreateInfo fenceCreateInfo = {};
	fenceCreateInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
	fenceCreateInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;
	fenceCreateInfo.pNext = nullptr;
	m_WaitFences.resize(m_SwapchainImages.size());
	for (VkFence& fence : m_WaitFences)
	{
		vkCreateFence(m_Device, &fenceCreateInfo, nullptr, &fence);
	}
}

void Graphics::CreateCommandBuffers()
{
	m_CommandBuffers.resize(m_SwapchainImages.size());

	for (auto& pCommandBuffer : m_CommandBuffers)
	{
		pCommandBuffer = std::make_unique<CommandBuffer>(this);
	}
}

void Graphics::CreateCommandPool()
{
	//Create command pool
	VkCommandPoolCreateInfo commandPoolCreateInfo = {};
	commandPoolCreateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
	commandPoolCreateInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
	commandPoolCreateInfo.pNext = nullptr;
	commandPoolCreateInfo.queueFamilyIndex = m_QueueFamilyIndex;
	VulkanTranslateError(vkCreateCommandPool(m_Device, &commandPoolCreateInfo, nullptr, &m_CommandPool));
}

void Graphics::CreateSwapchain()
{
	//Create surface
	VkWin32SurfaceCreateInfoKHR surfaceCreateInfo = {};
	surfaceCreateInfo.sType = VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR;
	surfaceCreateInfo.pNext = nullptr;
	surfaceCreateInfo.hinstance = GetModuleHandle(NULL);
	surfaceCreateInfo.hwnd = GetWindow();
	VulkanTranslateError(vkCreateWin32SurfaceKHR(m_Instance, &surfaceCreateInfo, nullptr, &m_Surface));

	//Check swapchain present support
	std::vector<VkBool32> supportsPresent(m_QueueFamilyProperties.size());
	for (size_t i = 0; i < m_QueueFamilyProperties.size(); ++i)
	{
		vkGetPhysicalDeviceSurfaceSupportKHR(m_PhysicalDevice, (unsigned int)i, m_Surface, &supportsPresent[i]);
	}

	unsigned int graphicsQueueFamilyIndex = UINT32_MAX;
	unsigned int presentQueueFamilyIndex = UINT32_MAX;
	for (size_t i = 0; i < m_QueueFamilyProperties.size(); ++i)
	{
		if ((m_QueueFamilyProperties[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) != 0)
		{
			if (graphicsQueueFamilyIndex == UINT32_MAX)
			{
				graphicsQueueFamilyIndex = (uint32)i;
			}
			if (supportsPresent[i] == VK_TRUE)
			{
				graphicsQueueFamilyIndex = (unsigned int)i;
				presentQueueFamilyIndex = (unsigned int)i;
				break;
			}
		}
	}
	if (presentQueueFamilyIndex == UINT32_MAX)
	{
		for (size_t i = 0; i < m_QueueFamilyProperties.size(); ++i)
		{
			if (supportsPresent[i] == VK_TRUE)
			{
				presentQueueFamilyIndex = (unsigned int)i;
			}
		}
	}

	//Create swapchain
	VkSurfaceCapabilitiesKHR surfaceCapabilities;
	vkGetPhysicalDeviceSurfaceCapabilitiesKHR(m_PhysicalDevice, m_Surface, &surfaceCapabilities);
	unsigned int presentModeCount = 0;
	vkGetPhysicalDeviceSurfacePresentModesKHR(m_PhysicalDevice, m_Surface, &presentModeCount, nullptr);
	std::vector<VkPresentModeKHR> presentModes(presentModeCount);
	vkGetPhysicalDeviceSurfacePresentModesKHR(m_PhysicalDevice, m_Surface, &presentModeCount, presentModes.data());

	VkCompositeAlphaFlagBitsKHR compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
	VkCompositeAlphaFlagBitsKHR compositeAlphaFlags[4] = {
		VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR,
		VK_COMPOSITE_ALPHA_PRE_MULTIPLIED_BIT_KHR,
		VK_COMPOSITE_ALPHA_POST_MULTIPLIED_BIT_KHR,
		VK_COMPOSITE_ALPHA_INHERIT_BIT_KHR,
	};

	unsigned int surfaceFormatCount = 0;
	vkGetPhysicalDeviceSurfaceFormatsKHR(m_PhysicalDevice, m_Surface, &surfaceFormatCount, nullptr);
	std::vector<VkSurfaceFormatKHR> surfaceFormats(surfaceFormatCount);
	vkGetPhysicalDeviceSurfaceFormatsKHR(m_PhysicalDevice, m_Surface, &surfaceFormatCount, surfaceFormats.data());

	VkSwapchainCreateInfoKHR swapchainInfo = {};
	swapchainInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
	swapchainInfo.pNext = nullptr;
	swapchainInfo.surface = m_Surface;
	swapchainInfo.imageFormat = surfaceFormats[0].format;
	swapchainInfo.minImageCount = surfaceCapabilities.minImageCount;
	swapchainInfo.imageExtent.width = m_WindowWidth;
	swapchainInfo.imageExtent.height = m_WindowHeight;
	swapchainInfo.preTransform = surfaceCapabilities.currentTransform;
	swapchainInfo.presentMode = presentModes[0];
	swapchainInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
	swapchainInfo.queueFamilyIndexCount = 0;
	swapchainInfo.pQueueFamilyIndices = nullptr;
	swapchainInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
	swapchainInfo.compositeAlpha = compositeAlpha;
	swapchainInfo.imageArrayLayers = 1;
	swapchainInfo.imageColorSpace = surfaceFormats[0].colorSpace;
	swapchainInfo.clipped = true;
	unsigned int queueFamilyIndices[] = { graphicsQueueFamilyIndex, presentQueueFamilyIndex };
	if (graphicsQueueFamilyIndex != presentQueueFamilyIndex)
	{
		swapchainInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
		swapchainInfo.queueFamilyIndexCount = 2;
		swapchainInfo.pQueueFamilyIndices = queueFamilyIndices;
	}
	VulkanTranslateError(vkCreateSwapchainKHR(m_Device, &swapchainInfo, nullptr, &m_SwapChain));

	//Create swapchain images
	unsigned int swapchainImageCount = 0;
	vkGetSwapchainImagesKHR(m_Device, m_SwapChain, &swapchainImageCount, nullptr);
	std::vector<VkImage> swapchainImages(swapchainImageCount);
	vkGetSwapchainImagesKHR(m_Device, m_SwapChain, &swapchainImageCount, swapchainImages.data());

	//Create image
	m_SwapchainImages.resize(swapchainImages.size());
	for (size_t i = 0; i < m_SwapchainImages.size(); i++)
	{
		m_SwapchainImages[i] = new Texture2D(this);
		m_SwapchainImages[i]->SetSize(m_WindowWidth, m_WindowHeight, VK_FORMAT_B8G8R8A8_UNORM, VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT, 1, (int64)swapchainImages[i]);
	}
}

void Graphics::CreateDevice()
{
	//Enumerate Devices
	unsigned int deviceCount = 0;
	VulkanTranslateError(vkEnumeratePhysicalDevices(m_Instance, &deviceCount, nullptr));
	if (deviceCount == 0)
	{
		std::cout << "No compatible devices" << std::endl;
		return;
	}

	std::vector<VkPhysicalDevice> devices(deviceCount);
	VulkanTranslateError(vkEnumeratePhysicalDevices(m_Instance, &deviceCount, devices.data()));
	//Just take the first device for now
	m_PhysicalDevice = devices[0];

	vkGetPhysicalDeviceProperties(m_PhysicalDevice, &m_DeviceProperties);
	VkPhysicalDeviceFeatures deviceFeatures;
	vkGetPhysicalDeviceFeatures(m_PhysicalDevice, &deviceFeatures);

	//Create device
	unsigned int familyPropertiesCount = 0;
	vkGetPhysicalDeviceQueueFamilyProperties(m_PhysicalDevice, &familyPropertiesCount, nullptr);
	m_QueueFamilyProperties.resize(familyPropertiesCount);
	vkGetPhysicalDeviceQueueFamilyProperties(m_PhysicalDevice, &familyPropertiesCount, m_QueueFamilyProperties.data());
	vkGetPhysicalDeviceMemoryProperties(m_PhysicalDevice, &m_MemoryProperties);

	bool found = false;
	for (size_t i = 0; i < m_QueueFamilyProperties.size(); ++i)
	{
		if (m_QueueFamilyProperties[i].queueFlags & VK_QUEUE_GRAPHICS_BIT)
		{
			m_QueueFamilyIndex = (unsigned int)i;
			found = true;
			break;
		}
	}
	if (found == false)
	{
		std::cout << "The device does not support graphics!" << std::endl;
		return;
	}

	float queuePriorities[] = { 1.0f };
	VkDeviceQueueCreateInfo deviceQueueCreateInfo = {};
	deviceQueueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
	deviceQueueCreateInfo.flags = 0;
	deviceQueueCreateInfo.pNext = nullptr;
	deviceQueueCreateInfo.pQueuePriorities = queuePriorities;
	deviceQueueCreateInfo.queueFamilyIndex = m_QueueFamilyIndex;
	deviceQueueCreateInfo.queueCount = 1;

	std::vector<const char*> deviceExtensions;
	deviceExtensions.push_back(VK_KHR_SWAPCHAIN_EXTENSION_NAME);

	VkDeviceCreateInfo deviceCreateInfo = {};
	deviceCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
	deviceCreateInfo.flags = 0;
	deviceCreateInfo.pNext = nullptr;
	deviceCreateInfo.enabledExtensionCount = (uint32_t)deviceExtensions.size();
	deviceCreateInfo.ppEnabledExtensionNames = deviceExtensions.data();
	deviceCreateInfo.pEnabledFeatures = nullptr;
	deviceCreateInfo.enabledLayerCount = 0;
	deviceCreateInfo.ppEnabledLayerNames = nullptr;
	deviceCreateInfo.queueCreateInfoCount = 1;
	deviceCreateInfo.pQueueCreateInfos = &deviceQueueCreateInfo;

	VulkanTranslateError(vkCreateDevice(m_PhysicalDevice, &deviceCreateInfo, nullptr, &m_Device));
	vkGetDeviceQueue(m_Device, m_QueueFamilyIndex, 0, &m_DeviceQueue);
}

bool Graphics::CreateVulkanInstance()
{
	uint32_t version;
	vkEnumerateInstanceVersion(&version);

	//Create Instance
	VkApplicationInfo applicationInfo;
	applicationInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
	applicationInfo.apiVersion = version;
	applicationInfo.applicationVersion = 1;
	applicationInfo.engineVersion = 1;
	applicationInfo.pApplicationName = "VulkanBase";
	applicationInfo.pEngineName = "VulkanFramework";
	applicationInfo.pNext = nullptr;

	std::vector<const char*> extensionNames;
	extensionNames.push_back(VK_KHR_SURFACE_EXTENSION_NAME);
	extensionNames.push_back(VK_KHR_WIN32_SURFACE_EXTENSION_NAME);

	// On desktop the LunarG loaders exposes a meta layer that contains all layers
	std::vector<const char*> layerNames;
	layerNames.push_back("VK_LAYER_LUNARG_standard_validation");
	layerNames.push_back("VK_LAYER_LUNARG_core_validation");
	layerNames.push_back("VK_LAYER_RENDERDOC_Capture");
	if (CheckValidationLayerSupport(layerNames) == false)
	{
		return false;
	}

	VkInstanceCreateInfo createInfo;
	createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
	createInfo.flags = 0;
	createInfo.pNext = nullptr;
	createInfo.enabledLayerCount = (uint32_t)layerNames.size();
	createInfo.ppEnabledLayerNames = layerNames.data();;
	createInfo.pApplicationInfo = &applicationInfo;
	createInfo.enabledExtensionCount = (uint32_t)extensionNames.size();
	createInfo.ppEnabledExtensionNames = extensionNames.data();

	return vkCreateInstance(&createInfo, nullptr, &m_Instance) == VK_SUCCESS;
}

void Graphics::ConstructWindow()
{
	//Create window
	SDL_DisplayMode displayMode;
	SDL_GetCurrentDisplayMode(0, &displayMode);

	unsigned flags = SDL_WINDOW_RESIZABLE;
	m_pWindow = SDL_CreateWindow("VulkanBase", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, m_WindowWidth, m_WindowHeight, flags);
}

void Graphics::Gameloop()
{
	bool quit = false;
	while (quit == false)
	{
		SDL_Event event;
		while (SDL_PollEvent(&event))
		{
			switch (event.type)
			{
			case SDL_QUIT:
				quit = true;
			}
		}
		++m_FrameCount;
		Draw();
	}

	if (m_Device != VK_NULL_HANDLE)
	{
		vkDeviceWaitIdle(m_Device);
	}
}

bool Graphics::CheckValidationLayerSupport(const std::vector<const char*>& layers)
{
	uint32_t layerCount;
	vkEnumerateInstanceLayerProperties(&layerCount, nullptr);

	std::vector<VkLayerProperties> availableLayers(layerCount);
	vkEnumerateInstanceLayerProperties(&layerCount, availableLayers.data());

	bool success = true;
	for (const char* layer : layers)
	{
		auto it = std::find_if(availableLayers.begin(), availableLayers.end(), [layer](const VkLayerProperties& a) { return strcmp(a.layerName, layer) == 0; });
		if (it == availableLayers.end())
		{
			std::cout << "Layer '" << layer << "' is not available" << std::endl;
			success = false;
		}
	}

	return success;
}

void Graphics::UpdateUniforms()
{
	struct ModelBuffer
	{
		glm::mat4 ModelMatrix;
		glm::mat4 MvpMatrix;
	} ModelBufferData;

	m_ProjectionMatrix = glm::perspective(glm::radians(45.0f), 1240.0f / 720.0f, 0.1f, 100.0f);
	m_ViewMatrix = glm::lookAt(
		glm::vec3(-5, 3, -10), // Camera is at (-5,3,-10), in World Space
		glm::vec3(0, 0, 0),    // and looks at the origin
		glm::vec3(0, -1, 0)    // Head is up (set to 0,-1,0 to look upside-down)
	);

	for (size_t i = 0; i < m_Drawables.size(); ++i)
	{
		m_ModelMatrix = m_Drawables[i]->GetWorldMatrix();
		m_Drawables[i]->SetRotation(0.0f, (float)pow(-1, i), 0.0f, (float)m_FrameCount / 50.0f);
		m_Drawables[i]->SetPosition((float)pow(-1, i) * 2, (float)pow(-1, i) * sin((float)m_FrameCount / 50.0f), 0);
		m_MvpMatrix = m_ProjectionMatrix * m_ViewMatrix * m_ModelMatrix;

		ModelBufferData.ModelMatrix = m_ModelMatrix;
		ModelBufferData.MvpMatrix = m_MvpMatrix;

		m_pUniformBuffer->SetData(0, sizeof(ModelBuffer), &ModelBufferData);
	}
}

void Graphics::BuildCommandBuffers()
{
	VkViewport viewport;
	viewport.height = (float)m_WindowHeight;
	viewport.width = (float)m_WindowWidth;
	viewport.x = 0;
	viewport.y = 0;
	viewport.minDepth = 0.0f;
	viewport.maxDepth = 1.0f;

	for(size_t i = 0; i < m_CommandBuffers.size(); ++i)
	{
		m_CommandBuffers[i]->Begin();
		m_CommandBuffers[i]->BeginRenderPass(m_FrameBuffers[i], m_RenderPass, m_WindowWidth, m_WindowHeight);
		m_CommandBuffers[i]->SetViewport(viewport);
		m_CommandBuffers[i]->SetGraphicsPipeline(m_pMaterial->GetPipeline());

		for (size_t j = 0; j < m_Drawables.size(); ++j)
		{
			std::vector<unsigned int> d = { 0 };
			m_CommandBuffers[i]->SetVertexBuffer(0, m_Drawables[j]->GetVertexBuffer());
			m_CommandBuffers[i]->SetIndexBuffer(0, m_Drawables[j]->GetIndexBuffer());

			std::vector<unsigned int> dynamicOffsets = { (unsigned int)m_pUniformBuffer->GetOffset((int)j) };
			m_CommandBuffers[i]->SetDescriptorSets(m_pMaterial->GetPipelineLayout(), m_pMaterial->GetDescriptorSets(), dynamicOffsets);
			m_CommandBuffers[i]->DrawIndexed(m_Drawables[j]->GetIndexBuffer()->GetCount(), 0);
		}
		m_CommandBuffers[i]->EndRenderPass();
		m_CommandBuffers[i]->End();
	}
}

void Graphics::Draw()
{
	//Get swapchain buffer
	VulkanTranslateError(vkAcquireNextImageKHR(m_Device, m_SwapChain, UINT64_MAX, m_PresentCompleteSemaphore, VK_NULL_HANDLE, (uint32_t*)&m_CurrentBuffer));

	vkWaitForFences(m_Device, 1, &m_WaitFences[m_CurrentBuffer], VK_TRUE, UINT64_MAX);
	vkResetFences(m_Device, 1, &m_WaitFences[m_CurrentBuffer]);

	UpdateUniforms();
	m_pUniformBuffer->Flush();

	const VkCommandBuffer commandBuffers[] = { m_CommandBuffers[m_CurrentBuffer]->GetBuffer() };
	VkPipelineStageFlags pipelineStateFlags = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	VkSubmitInfo submitInfo[1] = {};
	submitInfo[0].pNext = nullptr;
	submitInfo[0].sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	submitInfo[0].waitSemaphoreCount = 1;
	submitInfo[0].pWaitSemaphores = &m_PresentCompleteSemaphore;
	submitInfo[0].pWaitDstStageMask = &pipelineStateFlags;
	submitInfo[0].commandBufferCount = 1;
	submitInfo[0].pCommandBuffers = commandBuffers;
	submitInfo[0].signalSemaphoreCount = 1;
	submitInfo[0].pSignalSemaphores = &m_RenderCompleteSemaphore;
	vkQueueSubmit(m_DeviceQueue, 1, submitInfo, m_WaitFences[m_CurrentBuffer]);

	VkPresentInfoKHR presentInfo = {};
	presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
	presentInfo.pNext = NULL;
	presentInfo.swapchainCount = 1;
	presentInfo.pSwapchains = &m_SwapChain;
	presentInfo.pImageIndices = (uint32_t*)&m_CurrentBuffer;
	presentInfo.pWaitSemaphores = &m_RenderCompleteSemaphore;
	presentInfo.waitSemaphoreCount = 1;
	presentInfo.pResults = NULL;
	vkQueuePresentKHR(m_DeviceQueue, &presentInfo);
}

void Graphics::Shutdown()
{
	m_pMaterial.reset();
	m_Drawables.clear();

	delete m_pUniformBuffer;
	delete m_pImageTexture;

	m_CommandBuffers.clear();

	vkDestroySemaphore(m_Device, m_PresentCompleteSemaphore, nullptr);
	vkDestroySemaphore(m_Device, m_RenderCompleteSemaphore, nullptr);
	for (VkFence& fence : m_WaitFences)
	{
		vkDestroyFence(m_Device, fence, nullptr);
	}

	for (Texture2D* view : m_SwapchainImages)
	{
		delete view;
	}
	delete m_pDepthTexture;
	vkDestroyCommandPool(m_Device, m_CommandPool, nullptr);

	vkDestroyRenderPass(m_Device, m_RenderPass, nullptr);
	vkDestroyPipelineCache(m_Device, m_PipelineCache, nullptr);
	m_pDescriptorPool.reset();

	for (size_t i = 0; i < m_FrameBuffers.size() ; i++)
	{
		vkDestroyFramebuffer(m_Device, m_FrameBuffers[i], nullptr);
	}

	vkDestroySwapchainKHR(m_Device, m_SwapChain, nullptr);
	vkDestroyDevice(m_Device, nullptr);
	vkDestroyInstance(m_Instance, nullptr);
}
