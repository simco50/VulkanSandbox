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
#include "VulkanAllocator.h"
#include "VulkanHelpers.h"

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

void Graphics::Initialize()
{
	ConstructWindow();
	CreateVulkanInstance();
	CreateDevice(m_Instance);
	m_pAllocator = new VulkanAllocator(m_PhysicalDevice, m_Device);

	CreateSwapchain();
	CreateCommandPool();
	CreateCommandBuffers();
	CreateSynchronizationPrimitives();
	CreatePipelineCache();
	CreateDescriptorPool();
	CreateGlobalPipelineLayout();
	CreateRenderPassAndFrameBuffer();

	m_pMaterial = std::make_unique<Material>(this);
	m_pMaterial->Load("Resources/Materials/Default.xml");

#pragma endregion

	std::unique_ptr<Cube> pCube = std::make_unique<Cube>(this);
	pCube->Create(1, 1, 1);
	pCube->SetPosition(-1.0f, 0.0f, 1.0f);
	pCube->SetMaterial(m_pMaterial.get());
	m_Drawables.push_back(std::move(pCube));

	pCube = std::make_unique<Cube>(this);
	pCube->Create(1, 0.5f, 1);
	pCube->SetPosition(2.0f, 0.0f, 1.0f);
	pCube->SetMaterial(m_pMaterial.get());
	m_Drawables.push_back(std::move(pCube));

	m_pUniformBuffer = new UniformBuffer(this);
	m_pUniformBuffer->SetSize(sizeof(glm::mat4) * 2);

	m_pUniformBufferPerFrame = new UniformBuffer(this);
	m_pUniformBufferPerFrame->SetSize(sizeof(float) + sizeof(int));

	VkDescriptorBufferInfo ubInfo;

	ubInfo = {};
	ubInfo.buffer = m_pUniformBuffer->GetBuffer();
	ubInfo.range = VK_WHOLE_SIZE;
	ubInfo.offset = 0;

	std::vector<VkWriteDescriptorSet> writes;
	VkWriteDescriptorSet write;
	write = {};
	write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	write.descriptorCount = 1;
	write.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC;
	write.dstBinding = (int)DescriptorBinding::ModelMatrices;
	write.dstSet = m_ObjectDescriptorSet;
	write.pBufferInfo = &ubInfo;
	write.pNext = nullptr;
	write.dstArrayElement = 0;
	writes.push_back(write);

	VkDescriptorBufferInfo ubInfo2;
	ubInfo2 = {};
	ubInfo2.buffer = m_pUniformBufferPerFrame->GetBuffer();
	ubInfo2.range = VK_WHOLE_SIZE;
	ubInfo2.offset = 0;

	write = {};
	write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	write.descriptorCount = 1;
	write.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	write.dstBinding = (int)DescriptorBinding::FrameData;
	write.dstSet = m_FrameDescriptorSet;
	write.pBufferInfo = &ubInfo2;
	write.pNext = nullptr;
	write.dstArrayElement = 0;
	writes.push_back(write);

	vkUpdateDescriptorSets(m_Device, (uint32)writes.size(), writes.data(), 0, nullptr);

	BuildCommandBuffers();

	Gameloop();
}

void Graphics::CopyBufferWithStaging(VkBuffer targetBuffer, void* pData)
{
	VkMemoryRequirements targetBufferRequirements;
	vkGetBufferMemoryRequirements(m_Device, targetBuffer, &targetBufferRequirements);

	VkBuffer stagingBuffer;

	VkBufferCreateInfo bufferCreateInfo = {};
	bufferCreateInfo.flags = 0;
	bufferCreateInfo.pNext = nullptr;
	bufferCreateInfo.pQueueFamilyIndices = nullptr;
	bufferCreateInfo.queueFamilyIndexCount = 0;
	bufferCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
	bufferCreateInfo.size = targetBufferRequirements.size;
	bufferCreateInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
	bufferCreateInfo.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
	vkCreateBuffer(m_Device, &bufferCreateInfo, nullptr, &stagingBuffer);

	VulkanAllocation allocation = m_pAllocator->Allocate(stagingBuffer, true);
	vkBindBufferMemory(m_Device, stagingBuffer, allocation.Memory, allocation.Offset);

	memcpy(allocation.pCpuPointer, pData, (size_t)targetBufferRequirements.size);

	VkBufferCopy copyRegion = {};
	copyRegion.size = targetBufferRequirements.size;

	std::unique_ptr<CommandBuffer> copyCmd = GetTempCommandBuffer(true);
	copyCmd->CopyBuffer(stagingBuffer, targetBuffer, (int)targetBufferRequirements.size);
	FlushCommandBuffer(copyCmd);

	allocation.Free();
	vkDestroyBuffer(m_Device, stagingBuffer, nullptr);
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

void Graphics::ConstructWindow()
{
	//Create window
	SDL_DisplayMode displayMode;
	SDL_GetCurrentDisplayMode(0, &displayMode);

	unsigned flags = SDL_WINDOW_RESIZABLE;
	m_pWindow = SDL_CreateWindow("VulkanBase", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, m_WindowWidth, m_WindowHeight, flags);
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

void Graphics::CreateDevice(VkInstance instance)
{
	//Enumerate Devices
	unsigned int deviceCount = 0;
	VK_LOG(vkEnumeratePhysicalDevices(instance, &deviceCount, nullptr));
	if (deviceCount == 0)
	{
		std::cout << "No compatible devices" << std::endl;
		return;
	}

	std::vector<VkPhysicalDevice> devices(deviceCount);
	VK_LOG(vkEnumeratePhysicalDevices(instance, &deviceCount, devices.data()));
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

	VK_LOG(vkCreateDevice(m_PhysicalDevice, &deviceCreateInfo, nullptr, &m_Device));
	vkGetDeviceQueue(m_Device, m_QueueFamilyIndex, 0, &m_DeviceQueue);
}

void Graphics::CreateSwapchain()
{
	//Create surface
	VkWin32SurfaceCreateInfoKHR surfaceCreateInfo = {};
	surfaceCreateInfo.sType = VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR;
	surfaceCreateInfo.pNext = nullptr;
	surfaceCreateInfo.hinstance = GetModuleHandle(NULL);
	surfaceCreateInfo.hwnd = GetWindow();
	VK_LOG(vkCreateWin32SurfaceKHR(m_Instance, &surfaceCreateInfo, nullptr, &m_Surface));

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

	unsigned int imageCount = Clamp(2u, surfaceCapabilities.minImageCount, surfaceCapabilities.maxImageCount);

	VkSwapchainCreateInfoKHR swapchainInfo = {};
	swapchainInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
	swapchainInfo.pNext = nullptr;
	swapchainInfo.surface = m_Surface;
	swapchainInfo.imageFormat = surfaceFormats[0].format;
	swapchainInfo.minImageCount = imageCount;
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
	VK_LOG(vkCreateSwapchainKHR(m_Device, &swapchainInfo, nullptr, &m_SwapChain));

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

	//Depthstencil buffer
	m_pDepthTexture = new Texture2D(this);
	m_pDepthTexture->SetSize(m_WindowWidth, m_WindowHeight, VK_FORMAT_D32_SFLOAT, VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT, 1, 0);
}

void Graphics::CreateCommandPool()
{
	//Create command pool
	VkCommandPoolCreateInfo commandPoolCreateInfo = {};
	commandPoolCreateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
	commandPoolCreateInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
	commandPoolCreateInfo.pNext = nullptr;
	commandPoolCreateInfo.queueFamilyIndex = m_QueueFamilyIndex;
	VK_LOG(vkCreateCommandPool(m_Device, &commandPoolCreateInfo, nullptr, &m_CommandPool));
}

void Graphics::CreateCommandBuffers()
{
	m_CommandBuffers.resize(m_SwapchainImages.size());

	for (auto& pCommandBuffer : m_CommandBuffers)
	{
		pCommandBuffer = std::make_unique<CommandBuffer>(this);
	}
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

void Graphics::CreateDescriptorPool()
{
	m_pDescriptorPool = std::make_unique<DescriptorPool>(this);
}

void Graphics::CreatePipelineCache()
{
	std::vector<char> data;

	/*std::ifstream str("pipelinecache.bin", std::ios::binary | std::ios::ate);
	if (!str.fail())
	{
	data.resize((size_t)str.tellg());
	str.seekg(0);
	str.read(data.data(), data.size());
	str.close();
	}*/

	VkPipelineCacheCreateInfo cacheCreateInfo = {};
	cacheCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_CACHE_CREATE_INFO;
	cacheCreateInfo.pNext = nullptr;
	cacheCreateInfo.flags = 0;
	cacheCreateInfo.initialDataSize = data.size();
	cacheCreateInfo.pInitialData = data.data();
	vkCreatePipelineCache(m_Device, &cacheCreateInfo, nullptr, &m_PipelineCache);
}

void Graphics::UnloadPipelineCache()
{
	/*size_t size = 0;
	vkGetPipelineCacheData(m_Device, m_PipelineCache, &size, nullptr);
	std::vector<char> data(size);
	vkGetPipelineCacheData(m_Device, m_PipelineCache, &size, data.data());
	std::ofstream str("pipelinecache.bin", std::ios::binary);
	str.write(data.data(), data.size());
	str.close();*/

	vkDestroyPipelineCache(m_Device, m_PipelineCache, nullptr);
}

void Graphics::CreateRenderPassAndFrameBuffer()
{
	//Create render pass
	//Attachments
	std::vector<VkAttachmentDescription> attachments;
	attachments.push_back(VkHelpers::AttachmentDescriptor::ConstructColor(VK_FORMAT_B8G8R8A8_UNORM, 1));
	attachments.push_back(VkHelpers::AttachmentDescriptor::ConstructDepth(VK_FORMAT_D32_SFLOAT, 1));

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
	renderPassInfo.attachmentCount = attachments.size();
	renderPassInfo.dependencyCount = 0;
	renderPassInfo.flags = 0;
	renderPassInfo.pAttachments = attachments.data();
	renderPassInfo.pDependencies = nullptr;
	renderPassInfo.pNext = nullptr;
	renderPassInfo.pSubpasses = &subPassDescription;
	renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
	renderPassInfo.subpassCount = 1;
	vkCreateRenderPass(m_Device, &renderPassInfo, nullptr, &m_RenderPass);

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
		VK_LOG(vkCreateFramebuffer(m_Device, &frameBufferCreateInfo, nullptr, &m_FrameBuffers[i]));
	}
}

void Graphics::CreateGlobalPipelineLayout()
{
	m_DescriptorSetLayouts.resize((size_t)DescriptorGroup::MAX);
	std::vector<VkDescriptorSetLayoutBinding> bindings;
	VkDescriptorSetLayoutBinding binding;

	//PerFrame
	binding.binding = (int)DescriptorBinding::FrameData;
	binding.descriptorCount = 1;
	binding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	binding.pImmutableSamplers = nullptr;
	binding.stageFlags = VkShaderStageFlagBits::VK_SHADER_STAGE_ALL_GRAPHICS;
	bindings.push_back(binding);
	binding.binding = 1;
	binding.descriptorCount = 1;
	binding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	binding.pImmutableSamplers = nullptr;
	binding.stageFlags = VkShaderStageFlagBits::VK_SHADER_STAGE_FRAGMENT_BIT;
	bindings.push_back(binding);

	VkDescriptorSetLayoutCreateInfo descriptorSetLayoutCreateInfo = {};
	descriptorSetLayoutCreateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
	descriptorSetLayoutCreateInfo.pNext = nullptr;
	descriptorSetLayoutCreateInfo.pBindings = bindings.data();
	descriptorSetLayoutCreateInfo.bindingCount = bindings.size();

	vkCreateDescriptorSetLayout(m_Device, &descriptorSetLayoutCreateInfo, nullptr, &m_DescriptorSetLayouts[0]);

	//PerView
	bindings.clear();
	binding.binding = 0;
	binding.descriptorCount = 1;
	binding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC;
	binding.pImmutableSamplers = nullptr;
	binding.stageFlags = VkShaderStageFlagBits::VK_SHADER_STAGE_ALL_GRAPHICS;
	bindings.push_back(binding);

	descriptorSetLayoutCreateInfo.pBindings = bindings.data();
	descriptorSetLayoutCreateInfo.bindingCount = bindings.size();

	vkCreateDescriptorSetLayout(m_Device, &descriptorSetLayoutCreateInfo, nullptr, &m_DescriptorSetLayouts[1]);

	//PerObject
	bindings.clear();
	binding.binding = 0;
	binding.descriptorCount = 1;
	binding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC;
	binding.pImmutableSamplers = nullptr;
	binding.stageFlags = VkShaderStageFlagBits::VK_SHADER_STAGE_ALL_GRAPHICS;
	bindings.push_back(binding);

	descriptorSetLayoutCreateInfo.pBindings = bindings.data();
	descriptorSetLayoutCreateInfo.bindingCount = bindings.size();

	vkCreateDescriptorSetLayout(m_Device, &descriptorSetLayoutCreateInfo, nullptr, &m_DescriptorSetLayouts[2]);

	//PerMaterial
	bindings.clear();
	binding.binding = 1;
	binding.descriptorCount = 1;
	binding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	binding.pImmutableSamplers = nullptr;
	binding.stageFlags = VkShaderStageFlagBits::VK_SHADER_STAGE_FRAGMENT_BIT;
	bindings.push_back(binding);

	descriptorSetLayoutCreateInfo.pBindings = bindings.data();
	descriptorSetLayoutCreateInfo.bindingCount = bindings.size();

	vkCreateDescriptorSetLayout(m_Device, &descriptorSetLayoutCreateInfo, nullptr, &m_DescriptorSetLayouts[3]);


	//Create pipeline layout
	VkPipelineLayoutCreateInfo pipelineLayoutCreateInfo = VkHelpers::PipelineLayoutDescriptor();
	pipelineLayoutCreateInfo.setLayoutCount = m_DescriptorSetLayouts.size();
	pipelineLayoutCreateInfo.pSetLayouts = m_DescriptorSetLayouts.data();
	pipelineLayoutCreateInfo.pPushConstantRanges = nullptr;
	pipelineLayoutCreateInfo.pushConstantRangeCount = 0;
	vkCreatePipelineLayout(m_Device, &pipelineLayoutCreateInfo, nullptr, &m_PipelineLayout);

	m_ObjectDescriptorSet = m_pDescriptorPool->Allocate(m_DescriptorSetLayouts[(int)DescriptorGroup::Object]);
	m_FrameDescriptorSet = m_pDescriptorPool->Allocate(m_DescriptorSetLayouts[(int)DescriptorGroup::Frame]);
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
		m_Drawables[i]->SetRotation(0.0f, (float)pow(-1, i), 0.0f, (float)m_FrameCount / 50.0f);
		m_Drawables[i]->SetPosition((float)pow(-1, i) * 2, (float)pow(-1, i) * sin((float)m_FrameCount / 50.0f), 0);

		ModelBufferData.ModelMatrix = m_Drawables[i]->GetWorldMatrix();
		ModelBufferData.MvpMatrix = m_ProjectionMatrix * m_ViewMatrix * ModelBufferData.ModelMatrix;

		m_pUniformBuffer->SetData(0, sizeof(ModelBuffer), &ModelBufferData);
	}
	
	struct PerFrameData
	{
		float dt;
		int frameCount;
	} perFrameData;

	perFrameData.dt = 0.016f;
	perFrameData.frameCount = m_FrameCount;
	m_pUniformBufferPerFrame->SetData(0, sizeof(float) + sizeof(int), &perFrameData);
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

	for (size_t i = 0; i < m_CommandBuffers.size(); ++i)
	{
		m_CommandBuffers[i]->Begin();
		m_CommandBuffers[i]->BeginRenderPass(m_FrameBuffers[i], m_RenderPass, m_WindowWidth, m_WindowHeight);
		m_CommandBuffers[i]->SetViewport(viewport);
		m_CommandBuffers[i]->SetGraphicsPipeline(m_pMaterial->GetPipeline());

		m_CommandBuffers[i]->SetDescriptorSet(m_PipelineLayout, (int)DescriptorGroup::Frame, m_FrameDescriptorSet, {});
		
		m_CommandBuffers[i]->SetDescriptorSet(m_PipelineLayout, (int)DescriptorGroup::Material, m_pMaterial->GetDescriptorSet(), {});
		for (size_t j = 0; j < m_Drawables.size(); ++j)
		{
			m_CommandBuffers[i]->SetVertexBuffer(0, m_Drawables[j]->GetVertexBuffer());
			m_CommandBuffers[i]->SetIndexBuffer(0, m_Drawables[j]->GetIndexBuffer());

			m_CommandBuffers[i]->SetDescriptorSet(m_PipelineLayout, (int)DescriptorGroup::Object, m_ObjectDescriptorSet, { (unsigned int)m_pUniformBuffer->GetOffset((int)j) });
			m_CommandBuffers[i]->DrawIndexed(m_Drawables[j]->GetIndexBuffer()->GetCount(), 0);
		}
		m_CommandBuffers[i]->EndRenderPass();
		m_CommandBuffers[i]->End();
	}
}

void Graphics::Draw()
{
	//Get swapchain buffer
	VK_LOG(vkAcquireNextImageKHR(m_Device, m_SwapChain, UINT64_MAX, m_PresentCompleteSemaphore, VK_NULL_HANDLE, (uint32_t*)&m_CurrentBuffer));

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
	delete m_pUniformBufferPerFrame;

	delete m_pAllocator;

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
	UnloadPipelineCache();

	vkDestroyPipelineLayout(m_Device, m_PipelineLayout, nullptr);
	for (size_t i = 0; i < m_DescriptorSetLayouts.size(); ++i)
	{
		vkDestroyDescriptorSetLayout(m_Device, m_DescriptorSetLayouts[i], nullptr);
	}

	m_pDescriptorPool.reset();

	for (size_t i = 0; i < m_FrameBuffers.size() ; i++)
	{
		vkDestroyFramebuffer(m_Device, m_FrameBuffers[i], nullptr);
	}

	vkDestroySwapchainKHR(m_Device, m_SwapChain, nullptr);
	vkDestroyDevice(m_Device, nullptr);
	vkDestroyInstance(m_Instance, nullptr);
}

VkDescriptorSet Graphics::GetDestriptorSet(DescriptorGroup group)
{
	return m_pDescriptorPool->Allocate(m_DescriptorSetLayouts[(int)group]);
}
