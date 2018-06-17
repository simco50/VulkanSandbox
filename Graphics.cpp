#include "stdafx.h"
#include "Graphics.h"

#include <SDL_syswm.h>
#include "Shader.h"
#include "UniformBuffer.h"
#include "VertexBuffer.h"
#include "IndexBuffer.h"
#include "Texture2D.h"

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
	CreateFrameBuffer();

	//Specific initialization

	//Create descriptor set layout
	std::vector<VkDescriptorSetLayoutBinding> layoutBindings(2);
	layoutBindings[0].binding = 0;
	layoutBindings[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	layoutBindings[0].descriptorCount = 1;
	layoutBindings[0].stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
	layoutBindings[0].pImmutableSamplers = nullptr;

	layoutBindings[1].binding = 1;
	layoutBindings[1].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	layoutBindings[1].pImmutableSamplers = nullptr;
	layoutBindings[1].descriptorCount = 1;
	layoutBindings[1].stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

	VkDescriptorSetLayoutCreateInfo descriptorLayoutCreateInfo = {};
	descriptorLayoutCreateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
	descriptorLayoutCreateInfo.pNext = nullptr;
	descriptorLayoutCreateInfo.flags = 0;
	descriptorLayoutCreateInfo.bindingCount = (uint32)layoutBindings.size();
	descriptorLayoutCreateInfo.pBindings = layoutBindings.data();
	VulkanTranslateError(vkCreateDescriptorSetLayout(m_Device, &descriptorLayoutCreateInfo, nullptr, &m_DescriptorSetLayout));

	//Create pipeline layout
	VkPipelineLayoutCreateInfo pipelineLayoutCreateInfo = {};
	pipelineLayoutCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
	pipelineLayoutCreateInfo.flags = 0;
	pipelineLayoutCreateInfo.pNext = nullptr;
	pipelineLayoutCreateInfo.pushConstantRangeCount = 0;
	pipelineLayoutCreateInfo.pPushConstantRanges = nullptr;
	pipelineLayoutCreateInfo.setLayoutCount = 1;
	pipelineLayoutCreateInfo.pSetLayouts = &m_DescriptorSetLayout;
	VulkanTranslateError(vkCreatePipelineLayout(m_Device, &pipelineLayoutCreateInfo, nullptr, &m_PipelineLayout));

	//Create descriptor pool
	std::vector<VkDescriptorPoolSize> descriptorPoolSizes(2);
	descriptorPoolSizes[0].descriptorCount = 1;
	descriptorPoolSizes[0].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	descriptorPoolSizes[1].descriptorCount = 1;
	descriptorPoolSizes[1].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;

	VkDescriptorPoolCreateInfo descriptorPoolCreateInfo = {};
	descriptorPoolCreateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
	descriptorPoolCreateInfo.flags = 0;
	descriptorPoolCreateInfo.maxSets = 1;
	descriptorPoolCreateInfo.pNext = nullptr;
	descriptorPoolCreateInfo.poolSizeCount = (uint32)descriptorPoolSizes.size();
	descriptorPoolCreateInfo.pPoolSizes = descriptorPoolSizes.data();
	VulkanTranslateError(vkCreateDescriptorPool(m_Device, &descriptorPoolCreateInfo, nullptr, &m_DescriptorPool));

	//Allocate descriptor set
	VkDescriptorSetAllocateInfo descriptorAllocateInfo[1];
	descriptorAllocateInfo[0].sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
	descriptorAllocateInfo[0].descriptorPool = m_DescriptorPool;
	descriptorAllocateInfo[0].descriptorSetCount = 1;
	descriptorAllocateInfo[0].pNext = nullptr;
	descriptorAllocateInfo[0].pSetLayouts = &m_DescriptorSetLayout;
	m_DescriptorSets.resize(1);
	VulkanTranslateError(vkAllocateDescriptorSets(m_Device, descriptorAllocateInfo, m_DescriptorSets.data()));

	Shader* pShader = new Shader(m_Device);
	if (pShader->Load("Resources/Shaders/vert.spv", VK_SHADER_STAGE_VERTEX_BIT) == false)
	{
		return;
	}
	m_Shaders.push_back(pShader);
	pShader = new Shader(m_Device);
	pShader->Load("Resources/Shaders/frag.spv", VK_SHADER_STAGE_FRAGMENT_BIT);
	m_Shaders.push_back(pShader);

	//Vertex buffer
	std::vector<SampleVertex> vertices = 
	{
		//front 0
		{ glm::vec3(-1.0f, 1.0f, -1.0f), glm::vec3(0.0f, 0.0f, -1.0f), glm::vec2(0, 0) },
		{ glm::vec3(1.0f, 1.0f, -1.0f), glm::vec3(0.0f, 0.0f, -1.0f), glm::vec2(1, 0) },
		{ glm::vec3(1.0f, -1.0f, -1.0f), glm::vec3(0.0f, 0.0f, -1.0f), glm::vec2(1, 1) },
		{ glm::vec3(-1.0f, -1.0f, -1.0f), glm::vec3(0.0f, 0.0f, -1.0f), glm::vec2(1, 0) },

		//top 4
		{ glm::vec3(-1.0f, 1.0f, -1.0f), glm::vec3(0.0f, 1.0f, 0.0f), glm::vec2(0, 0) },
		{ glm::vec3(-1.0f, 1.0f, 1.0f), glm::vec3(0.0f, 1.0f, 0.0f), glm::vec2(1, 0) },
		{ glm::vec3(1.0f, 1.0f, 1.0f), glm::vec3(0.0f, 1.0f, 0.0f) , glm::vec2(1, 1) },
		{ glm::vec3(1.0f, 1.0f, -1.0f), glm::vec3(0.0f, 1.0f, 0.0f), glm::vec2(1, 0) },

		//left 8
		{ glm::vec3(-1.0f, 1.0f, -1.0f), glm::vec3(-1.0f, 0.0f, 0.0f), glm::vec2(0, 0) },
		{ glm::vec3(-1.0f, 1.0f, 1.0f), glm::vec3(-1.0f, 0.0f, 0.0f), glm::vec2(1, 0) },
		{ glm::vec3(-1.0f, -1.0f, 1.0f), glm::vec3(-1.0f, 0.0f, 0.0f), glm::vec2(1, 1) },
		{ glm::vec3(-1.0f, -1.0f, -1.0f), glm::vec3(-1.0f, 0.0f, 0.0f), glm::vec2(1, 0) },

		//right 12
		{ glm::vec3(1.0f, 1.0f, -1.0f), glm::vec3(1.0f, 0.0f, 0.0f), glm::vec2(0, 0) },
		{ glm::vec3(1.0f, 1.0f, 1.0f), glm::vec3(1.0f, 0.0f, 0.0f), glm::vec2(1, 0) },
		{ glm::vec3(1.0f, -1.0f, 1.0f), glm::vec3(1.0f, 0.0f, 0.0f), glm::vec2(1, 1) },
		{ glm::vec3(1.0f, -1.0f, -1.0f), glm::vec3(1.0f, 0.0f, 0.0f), glm::vec2(1, 0) },

		//bottom 16
		{ glm::vec3(-1.0f, -1.0f, -1.0f), glm::vec3(0.0f, -1.0f, 0.0f), glm::vec2(0, 0) },
		{ glm::vec3(-1.0f, -1.0f, 1.0f), glm::vec3(0.0f, -1.0f, 0.0f), glm::vec2(1, 0) },
		{ glm::vec3(1.0f, -1.0f, 1.0f), glm::vec3(0.0f, -1.0f, 0.0f), glm::vec2(1, 1) },
		{ glm::vec3(1.0f, -1.0f, -1.0f), glm::vec3(0.0f, -1.0f, 0.0f), glm::vec2(1, 0) },

		//back 20
		{ glm::vec3(-1.0f, 1.0f, 1.0f), glm::vec3(0.0f, 0.0f, 1.0f), glm::vec2(0, 0) },
		{ glm::vec3(1.0f, 1.0f, 1.0f), glm::vec3(0.0f, 0.0f, 1.0f), glm::vec2(1, 0) },
		{ glm::vec3(1.0f, -1.0f, 1.0f) , glm::vec3(0.0f, 0.0f, 1.0f), glm::vec2(1, 1) },
		{ glm::vec3(-1.0f, -1.0f, 1.0f), glm::vec3(0.0f, 0.0f, 1.0f), glm::vec2(1, 0) },
	};

	std::vector<unsigned int> indices = {
		0, 1, 2,
		0, 2, 3,

		4, 5, 6,
		4, 6, 7,

		9, 8, 10,
		10, 8, 11,

		12, 13, 14,
		14, 15, 12,

		16, 18, 17,
		19, 18, 16,

		20, 23, 22,
		21, 20, 22
	};

	m_pVertexBuffer = new VertexBuffer(this);
	m_pVertexBuffer->SetSize((int)vertices.size() * sizeof(SampleVertex), false);
	m_pVertexBuffer->SetData((int)vertices.size() * sizeof(SampleVertex), 0, vertices.data());

	//Vertex layout
	VkVertexInputBindingDescription bindingDesc;
	bindingDesc.binding = 0;
	bindingDesc.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
	bindingDesc.stride = sizeof(SampleVertex);
	VkVertexInputAttributeDescription attributeDesc[3];
	int offset = 0;
	attributeDesc[0].binding = 0;
	attributeDesc[0].location = 0;
	attributeDesc[0].format = VK_FORMAT_R32G32B32_SFLOAT;
	attributeDesc[0].offset = offset;
	offset += sizeof(glm::vec3);
	attributeDesc[1].binding = 0;
	attributeDesc[1].location = 1;
	attributeDesc[1].format = VK_FORMAT_R32G32B32_SFLOAT;
	attributeDesc[1].offset = offset;
	offset += sizeof(glm::vec3);
	attributeDesc[2].binding = 0;
	attributeDesc[2].location = 2;
	attributeDesc[2].format = VK_FORMAT_R32G32_SFLOAT;
	attributeDesc[2].offset = offset;

	//Pipeline vertex input state
	VkPipelineVertexInputStateCreateInfo vertexStateInfo = {};
	vertexStateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
	vertexStateInfo.pNext = nullptr;
	vertexStateInfo.flags = 0;
	vertexStateInfo.vertexBindingDescriptionCount = 1;
	vertexStateInfo.pVertexBindingDescriptions = &bindingDesc;
	vertexStateInfo.vertexAttributeDescriptionCount = 3;
	vertexStateInfo.pVertexAttributeDescriptions = attributeDesc;

	m_pIndexBuffer = new IndexBuffer(this);
	m_pIndexBuffer->SetSize((int)indices.size(), false, false);
	m_pIndexBuffer->SetData(indices.data());

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
	ubInfo.range = m_pUniformBuffer->GetSize();
	ubInfo.offset = 0;

	std::vector<VkWriteDescriptorSet> writes(2);
	writes[0] = {};
	writes[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	writes[0].descriptorCount = 1;
	writes[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	writes[0].dstBinding = 0;
	writes[0].dstSet = m_DescriptorSets[0];
	writes[0].pBufferInfo = &ubInfo;
	writes[0].pNext = nullptr;
	writes[0].dstArrayElement = 0;

	writes[1] = {};
	writes[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	writes[1].descriptorCount = 1;
	writes[1].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	writes[1].dstBinding = 1;
	writes[1].dstSet = m_DescriptorSets[0];
	writes[1].pImageInfo = &texInfo;
	writes[1].pNext = nullptr;
	writes[1].dstArrayElement = 0;
	vkUpdateDescriptorSets(m_Device, (uint32)writes.size(), writes.data(), 0, nullptr);

	//Dynamic state
	VkDynamicState dynamicStateEnables[VK_DYNAMIC_STATE_RANGE_SIZE];
	VkPipelineDynamicStateCreateInfo dynamicState = {};
	memset(dynamicStateEnables, 0, sizeof(dynamicStateEnables));
	dynamicState.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
	dynamicState.pNext = nullptr;
	dynamicState.pDynamicStates = dynamicStateEnables;
	dynamicState.dynamicStateCount = 0;

	//Pipeline vertex input assembly state
	VkPipelineInputAssemblyStateCreateInfo inputAssemblyInfo = {};
	inputAssemblyInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
	inputAssemblyInfo.pNext = nullptr;
	inputAssemblyInfo.flags = 0;
	inputAssemblyInfo.primitiveRestartEnable = VK_FALSE;
	inputAssemblyInfo.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;

	//Rasterization state
	VkPipelineRasterizationStateCreateInfo rasterizerStateInfo = {};
	rasterizerStateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
	rasterizerStateInfo.pNext = NULL;
	rasterizerStateInfo.flags = 0;
	rasterizerStateInfo.polygonMode = VK_POLYGON_MODE_FILL;
	rasterizerStateInfo.cullMode = VK_CULL_MODE_BACK_BIT;
	rasterizerStateInfo.frontFace = VK_FRONT_FACE_CLOCKWISE;
	rasterizerStateInfo.depthClampEnable = VK_FALSE;
	rasterizerStateInfo.rasterizerDiscardEnable = VK_FALSE;
	rasterizerStateInfo.depthBiasEnable = VK_FALSE;
	rasterizerStateInfo.depthBiasConstantFactor = 0;
	rasterizerStateInfo.depthBiasClamp = 0;
	rasterizerStateInfo.depthBiasSlopeFactor = 0;
	rasterizerStateInfo.lineWidth = 1.0f;

	//Blend state
	VkPipelineColorBlendStateCreateInfo blendStateInfo = {};
	blendStateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
	blendStateInfo.pNext = nullptr;
	blendStateInfo.flags = 0;
	VkPipelineColorBlendAttachmentState attState[1];
	attState[0].colorWriteMask = 0xf;
	attState[0].blendEnable = VK_FALSE;
	attState[0].alphaBlendOp = VK_BLEND_OP_ADD;
	attState[0].colorBlendOp = VK_BLEND_OP_ADD;
	attState[0].srcColorBlendFactor = VK_BLEND_FACTOR_ZERO;
	attState[0].dstColorBlendFactor = VK_BLEND_FACTOR_ZERO;
	attState[0].srcAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
	attState[0].dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
	blendStateInfo.attachmentCount = 1;
	blendStateInfo.pAttachments = attState;
	blendStateInfo.logicOpEnable = VK_FALSE;
	blendStateInfo.logicOp = VK_LOGIC_OP_NO_OP;
	blendStateInfo.blendConstants[0] = 1.0f;
	blendStateInfo.blendConstants[1] = 1.0f;
	blendStateInfo.blendConstants[2] = 1.0f;
	blendStateInfo.blendConstants[3] = 1.0f;

	//Viewport state
	VkPipelineViewportStateCreateInfo viewportStateInfo = {};
	viewportStateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
	viewportStateInfo.pNext = nullptr;
	viewportStateInfo.flags = 0;
	viewportStateInfo.viewportCount = 1;
	dynamicStateEnables[dynamicState.dynamicStateCount++] = VK_DYNAMIC_STATE_VIEWPORT;
	viewportStateInfo.scissorCount = 1;
	dynamicStateEnables[dynamicState.dynamicStateCount++] = VK_DYNAMIC_STATE_SCISSOR;
	viewportStateInfo.pScissors = nullptr;
	viewportStateInfo.pViewports = nullptr;

	//Depth stencil state
	VkPipelineDepthStencilStateCreateInfo depthStencilStateInfo = {};
	depthStencilStateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
	depthStencilStateInfo.pNext = nullptr;
	depthStencilStateInfo.flags = 0;
	depthStencilStateInfo.back.failOp = VK_STENCIL_OP_KEEP;
	depthStencilStateInfo.back.passOp = VK_STENCIL_OP_KEEP;
	depthStencilStateInfo.back.compareOp = VK_COMPARE_OP_ALWAYS;
	depthStencilStateInfo.back.compareMask = 0;
	depthStencilStateInfo.back.reference = 0;
	depthStencilStateInfo.back.depthFailOp = VK_STENCIL_OP_KEEP;
	depthStencilStateInfo.back.writeMask = 0;
	depthStencilStateInfo.depthTestEnable = VK_TRUE;
	depthStencilStateInfo.depthWriteEnable = VK_TRUE;
	depthStencilStateInfo.depthCompareOp = VK_COMPARE_OP_LESS_OR_EQUAL;
	depthStencilStateInfo.depthBoundsTestEnable = VK_FALSE;
	depthStencilStateInfo.minDepthBounds = 0.0f;
	depthStencilStateInfo.maxDepthBounds = 1.0f;
	depthStencilStateInfo.stencilTestEnable = false;
	depthStencilStateInfo.front = depthStencilStateInfo.back;

	//MSAA state
	VkPipelineMultisampleStateCreateInfo multiSampleStateInfo = {};
	multiSampleStateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
	multiSampleStateInfo.pNext = nullptr;
	multiSampleStateInfo.flags = 0;
	multiSampleStateInfo.pSampleMask = nullptr;
	multiSampleStateInfo.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
	multiSampleStateInfo.sampleShadingEnable = VK_FALSE;
	multiSampleStateInfo.alphaToCoverageEnable = VK_FALSE;
	multiSampleStateInfo.alphaToOneEnable = VK_FALSE;
	multiSampleStateInfo.minSampleShading = 0.0f;

	VkPipelineShaderStageCreateInfo shaderCreateInfos[] = 
	{ 
		m_Shaders[0]->GetPipelineCreateInfo(), 
		m_Shaders[1]->GetPipelineCreateInfo() 
	};

	//Graphics pipeline
	VkGraphicsPipelineCreateInfo pipelineInfo = {};
	pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
	pipelineInfo.pNext = nullptr;
	pipelineInfo.layout = m_PipelineLayout;
	pipelineInfo.basePipelineHandle = VK_NULL_HANDLE;
	pipelineInfo.basePipelineIndex = 0;
	pipelineInfo.flags = 0;
	pipelineInfo.pVertexInputState = &vertexStateInfo;
	pipelineInfo.pInputAssemblyState = &inputAssemblyInfo;
	pipelineInfo.pRasterizationState = &rasterizerStateInfo;
	pipelineInfo.pColorBlendState = &blendStateInfo;
	pipelineInfo.pTessellationState = nullptr;
	pipelineInfo.pMultisampleState = &multiSampleStateInfo;
	pipelineInfo.pDynamicState = &dynamicState;
	pipelineInfo.pViewportState = &viewportStateInfo;
	pipelineInfo.pDepthStencilState = &depthStencilStateInfo;
	pipelineInfo.pStages = shaderCreateInfos;
	pipelineInfo.stageCount = 2;
	pipelineInfo.renderPass = m_RenderPass;
	pipelineInfo.subpass = 0;

	VulkanTranslateError(vkCreateGraphicsPipelines(m_Device, m_PipelineCache, 1, &pipelineInfo, nullptr, &m_GraphicsPipeline));

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

	VkCommandBuffer copyCmd = GetCommandBuffer(true);
	vkCmdCopyBuffer(copyCmd, stagingBuffer.Buffer, targetBuffer, 1, &copyRegion);
	FlushCommandBuffer(copyCmd);

	vkDestroyBuffer(m_Device, stagingBuffer.Buffer, nullptr);
	vkFreeMemory(m_Device, stagingBuffer.Memory, nullptr);
}

VkCommandBuffer Graphics::GetCommandBuffer(const bool begin)
{
	VkCommandBuffer cmdBuffer;
	VkCommandBufferAllocateInfo cmdBufAllocateInfo = {};
	cmdBufAllocateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	cmdBufAllocateInfo.commandPool = m_CommandPool;
	cmdBufAllocateInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	cmdBufAllocateInfo.commandBufferCount = 1;

	vkAllocateCommandBuffers(m_Device, &cmdBufAllocateInfo, &cmdBuffer);

	// If requested, also start the new command buffer
	if (begin)
	{
		VkCommandBufferBeginInfo cmdBufInfo = {};
		cmdBufInfo.flags = 0;
		cmdBufInfo.pInheritanceInfo = nullptr;
		cmdBufInfo.pNext = nullptr;
		cmdBufInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
		vkBeginCommandBuffer(cmdBuffer, &cmdBufInfo);
	}

	return cmdBuffer;
}

void Graphics::FlushCommandBuffer(VkCommandBuffer cmdBuffer)
{
	vkEndCommandBuffer(cmdBuffer);
	VkSubmitInfo submitInfo = {};
	submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	submitInfo.pNext = nullptr;
	submitInfo.commandBufferCount = 1;
	submitInfo.pCommandBuffers = &cmdBuffer;
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
	vkWaitForFences(m_Device, 1, &fence, VK_TRUE, 1000);

	vkDestroyFence(m_Device, fence, nullptr);
	vkFreeCommandBuffers(m_Device, m_CommandPool, 1, &cmdBuffer);
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
	VkCommandBufferAllocateInfo commandBufferAllocateInfo = {};
	commandBufferAllocateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	commandBufferAllocateInfo.commandBufferCount = (uint32)m_CommandBuffers.size();
	commandBufferAllocateInfo.commandPool = m_CommandPool;
	commandBufferAllocateInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	commandBufferAllocateInfo.pNext = nullptr;
	VulkanTranslateError(vkAllocateCommandBuffers(m_Device, &commandBufferAllocateInfo, m_CommandBuffers.data()));
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

	VkPhysicalDeviceProperties properties;
	vkGetPhysicalDeviceProperties(m_PhysicalDevice, &properties);
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
		UpdateUniforms();

		Draw();
	}

	if (m_Device != VK_NULL_HANDLE)
	{
		vkDeviceWaitIdle(m_Device);
	}
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
	m_ModelMatrix = glm::rotate(m_ModelMatrix, glm::radians((float)1.0f), glm::vec3(0, 1, 0));
	m_MvpMatrix = m_ProjectionMatrix * m_ViewMatrix * m_ModelMatrix;

	ModelBufferData.ModelMatrix = m_ModelMatrix;
	ModelBufferData.MvpMatrix = m_MvpMatrix;

	m_pUniformBuffer->SetData(0, sizeof(ModelBuffer), &ModelBufferData);
}

void Graphics::BuildCommandBuffers()
{
	VkCommandBufferBeginInfo beginInfo = {};
	beginInfo.flags = 0;
	beginInfo.pInheritanceInfo = nullptr;
	beginInfo.pNext = nullptr;
	beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;

	/* We cannot bind the vertex buffer until we begin a renderpass */
	VkClearValue clearValues[2];
	clearValues[0].color.float32[0] = 0.2f;
	clearValues[0].color.float32[1] = 0.2f;
	clearValues[0].color.float32[2] = 0.2f;
	clearValues[0].color.float32[3] = 0.2f;
	clearValues[1].depthStencil.depth = 1.0f;
	clearValues[1].depthStencil.stencil = 0;

	VkRenderPassBeginInfo rpBegin = {};
	rpBegin.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
	rpBegin.pNext = NULL;
	rpBegin.renderPass = m_RenderPass;
	rpBegin.renderArea.offset.x = 0;
	rpBegin.renderArea.offset.y = 0;
	rpBegin.renderArea.extent.width = m_WindowWidth;
	rpBegin.renderArea.extent.height = m_WindowHeight;
	rpBegin.clearValueCount = 2;
	rpBegin.pClearValues = clearValues;

	VkViewport viewport;
	viewport.height = (float)m_WindowHeight;
	viewport.width = (float)m_WindowWidth;
	viewport.x = 0;
	viewport.y = 0;
	viewport.minDepth = 0.0f;
	viewport.maxDepth = 1.0f;

	VkRect2D scissor;
	scissor.extent.height = m_WindowHeight;
	scissor.extent.width = m_WindowWidth;
	scissor.offset.x = 0;
	scissor.offset.y = 0;

	for(size_t i = 0; i < m_CommandBuffers.size(); ++i)
	{
		rpBegin.framebuffer = m_FrameBuffers[i];
		vkBeginCommandBuffer(m_CommandBuffers[i], &beginInfo);
		vkCmdBeginRenderPass(m_CommandBuffers[i], &rpBegin, VK_SUBPASS_CONTENTS_INLINE);

		vkCmdSetViewport(m_CommandBuffers[i], 0, 1, &viewport);
		vkCmdSetScissor(m_CommandBuffers[i], 0, 1, &scissor);

		vkCmdBindDescriptorSets(m_CommandBuffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, m_PipelineLayout, 0, 1, m_DescriptorSets.data(), 0, nullptr);
		
		vkCmdBindPipeline(m_CommandBuffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, m_GraphicsPipeline);

		const VkDeviceSize offsets[1] = { 0 };
		vkCmdBindVertexBuffers(m_CommandBuffers[i], 0, 1, &m_pVertexBuffer->GetBuffer(), offsets);
		vkCmdBindIndexBuffer(m_CommandBuffers[i], m_pIndexBuffer->GetBuffer(), 0, VK_INDEX_TYPE_UINT32);

		vkCmdDrawIndexed(m_CommandBuffers[i], m_pIndexBuffer->GetCount(), 1, 0, 0, 0);
		vkCmdEndRenderPass(m_CommandBuffers[i]);
		vkEndCommandBuffer(m_CommandBuffers[i]);
	}
}

void Graphics::Draw()
{
	//Get swapchain buffer
	VulkanTranslateError(vkAcquireNextImageKHR(m_Device, m_SwapChain, UINT64_MAX, m_PresentCompleteSemaphore, VK_NULL_HANDLE, (uint32_t*)&m_CurrentBuffer));

	vkWaitForFences(m_Device, 1, &m_WaitFences[m_CurrentBuffer], VK_TRUE, UINT64_MAX);
	vkResetFences(m_Device, 1, &m_WaitFences[m_CurrentBuffer]);

	const VkCommandBuffer commandBuffers[] = { m_CommandBuffers[m_CurrentBuffer] };
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
	vkDestroyPipeline(m_Device, m_GraphicsPipeline, nullptr);
	vkDestroyPipelineLayout(m_Device, m_PipelineLayout, nullptr);
	vkDestroyDescriptorSetLayout(m_Device, m_DescriptorSetLayout, nullptr);
	delete m_pUniformBuffer;
	delete m_pVertexBuffer;
	delete m_pIndexBuffer;
	delete m_pImageTexture;

	vkFreeCommandBuffers(m_Device, m_CommandPool, (uint32)m_CommandBuffers.size(), m_CommandBuffers.data());

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

	for (Shader* pShader : m_Shaders)
	{
		delete pShader;
	}

	vkDestroyRenderPass(m_Device, m_RenderPass, nullptr);
	vkDestroyDescriptorPool(m_Device, m_DescriptorPool, nullptr);
	vkDestroyPipelineCache(m_Device, m_PipelineCache, nullptr);

	for (size_t i = 0; i < m_FrameBuffers.size() ; i++)
	{
		vkDestroyFramebuffer(m_Device, m_FrameBuffers[i], nullptr);
	}

	vkDestroySwapchainKHR(m_Device, m_SwapChain, nullptr);
	vkDestroyDevice(m_Device, nullptr);
	vkDestroyInstance(m_Instance, nullptr);
}
