#include "stdafx.h"
#include "Material.h"

#include "tinyxml2.h"
#include "Shader.h"
#include "Graphics.h"

Material::Material(Graphics* pGraphics) :
	m_pGraphics(pGraphics)
{

}

Material::~Material()
{
	vkDestroyPipelineLayout(m_pGraphics->GetDevice(), m_PipelineLayout, nullptr);
	vkDestroyRenderPass(m_pGraphics->GetDevice(), m_RenderPass, nullptr);
	vkDestroyPipeline(m_pGraphics->GetDevice(), m_Pipeline, nullptr);
	vkDestroyDescriptorSetLayout(m_pGraphics->GetDevice(), m_DescriptorSetLayout, nullptr);
	vkDestroyDescriptorPool(m_pGraphics->GetDevice(), m_DescriptorPool, nullptr);
}

void Material::GetTypeAndSizeFromString(const std::string& type, VkFormat& format, int& size)
{
	if (type == "float4")
	{
		format = VK_FORMAT_R32G32B32A32_SFLOAT;
		size = 16;
	}
	else if (type == "float3")
	{
		format = VK_FORMAT_R32G32B32_SFLOAT;
		size = 12;
	}
	else if (type == "float2")
	{
		format = VK_FORMAT_R32G32_SFLOAT;
		size = 8;
	}
}

VkShaderStageFlagBits Material::GetShaderStageFromString(const std::string& stage)
{
	if (stage == "vs")
	{
		return VK_SHADER_STAGE_VERTEX_BIT;
	}
	else if (stage == "ps")
	{
		return VK_SHADER_STAGE_FRAGMENT_BIT;
	}
	return (VkShaderStageFlagBits)0;
}

void Material::Load(const std::string& fileName)
{
	std::ifstream file(fileName, std::ios::ate);
	std::vector<char> data((size_t)file.tellg());
	file.seekg(0);
	file.read(data.data(), data.size());

	namespace XML = tinyxml2;

	XML::XMLDocument document;
	if (document.Parse((char*)data.data(), data.size()) != XML::XML_SUCCESS)
	{
		return;
	}

	XML::XMLElement* pRootNode = document.FirstChildElement();
	std::string name = pRootNode->Attribute("name");

	XML::XMLElement* pCurrent =	pRootNode->FirstChildElement("Shaders");
	XML::XMLElement* pShaderElement = pCurrent->FirstChildElement("Shader");
	while (pShaderElement != nullptr)
	{
		std::unique_ptr<Shader> pShader = std::make_unique<Shader>(m_pGraphics->GetDevice());
		
		std::string stage = pShaderElement->Attribute("type");
		VkShaderStageFlagBits shaderStage = GetShaderStageFromString(stage);

		pShader->Load(pShaderElement->Attribute("path"), shaderStage);
		m_Shaders.push_back(std::move(pShader));

		pShaderElement = pShaderElement->NextSiblingElement();
	}

	pCurrent = pRootNode->FirstChildElement("Pipeline");
	pCurrent = pCurrent->FirstChildElement("VertexLayout");
	XML::XMLElement* pVertexElement = pCurrent->FirstChildElement();
	

	std::vector<VkVertexInputAttributeDescription> attributeDesc;
	int offset = 0;
	while (pVertexElement != nullptr)
	{
		int size = 0;
		VkFormat format;
		GetTypeAndSizeFromString(pVertexElement->Attribute("type"), format, size);

		VkVertexInputAttributeDescription desc;
		desc.binding = 0;
		desc.location = (unsigned int)attributeDesc.size();
		desc.offset = offset;
		desc.format = format;
		attributeDesc.push_back(desc);

		offset += size;

		pVertexElement = pVertexElement->NextSiblingElement();
	}
	VkVertexInputBindingDescription bindingDesc;
	bindingDesc.binding = 0;
	bindingDesc.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
	bindingDesc.stride = offset;

	//Pipeline vertex input state
	VkPipelineVertexInputStateCreateInfo vertexStateInfo = {};
	vertexStateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
	vertexStateInfo.pNext = nullptr;
	vertexStateInfo.flags = 0;
	vertexStateInfo.vertexBindingDescriptionCount = 1;
	vertexStateInfo.pVertexBindingDescriptions = &bindingDesc;
	vertexStateInfo.vertexAttributeDescriptionCount = (unsigned int)attributeDesc.size();
	vertexStateInfo.pVertexAttributeDescriptions = attributeDesc.data();


	pCurrent = pRootNode->FirstChildElement("Pipeline");
	pCurrent = pCurrent->FirstChildElement("Descriptors");

	std::vector<VkDescriptorSetLayoutBinding> layoutBindings;
	
	XML::XMLElement* pDescriptor = pCurrent->FirstChildElement();
	while (pDescriptor != nullptr)
	{
		VkDescriptorSetLayoutBinding binding;

		std::string type = pDescriptor->Name();
		std::string name = pDescriptor->Attribute("name");
		bool dynamic = pDescriptor->Attribute("dynamic") ? true : false;

		binding.binding = stoi(std::string(pDescriptor->Attribute("binding")));
		binding.descriptorCount = 1;
		binding.stageFlags = GetShaderStageFromString(pDescriptor->Attribute("shaderstage"));
		binding.pImmutableSamplers = nullptr;

		if (type == "UniformBuffer")
		{
			binding.descriptorType = dynamic ? VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC : VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		}
		else if (type == "Texture2D")
		{
			binding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		}

		layoutBindings.push_back(binding);
		pDescriptor = pDescriptor->NextSiblingElement();
	}

	VkDescriptorSetLayoutCreateInfo descriptorLayoutCreateInfo = {};
	descriptorLayoutCreateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
	descriptorLayoutCreateInfo.pNext = nullptr;
	descriptorLayoutCreateInfo.flags = 0;
	descriptorLayoutCreateInfo.bindingCount = (uint32)layoutBindings.size();
	descriptorLayoutCreateInfo.pBindings = layoutBindings.data();
	vkCreateDescriptorSetLayout(m_pGraphics->GetDevice(), &descriptorLayoutCreateInfo, nullptr, &m_DescriptorSetLayout);

	//Create pipeline layout
	VkPipelineLayoutCreateInfo pipelineLayoutCreateInfo = {};
	pipelineLayoutCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
	pipelineLayoutCreateInfo.flags = 0;
	pipelineLayoutCreateInfo.pNext = nullptr;
	pipelineLayoutCreateInfo.pushConstantRangeCount = 0;
	pipelineLayoutCreateInfo.pPushConstantRanges = nullptr;
	pipelineLayoutCreateInfo.setLayoutCount = 1;
	pipelineLayoutCreateInfo.pSetLayouts = &m_DescriptorSetLayout;
	vkCreatePipelineLayout(m_pGraphics->GetDevice(), &pipelineLayoutCreateInfo, nullptr, &m_PipelineLayout);

	//Create descriptor pool
	std::vector<VkDescriptorPoolSize> descriptorPoolSizes(2);
	descriptorPoolSizes[0].descriptorCount = 12;
	descriptorPoolSizes[0].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC;
	descriptorPoolSizes[1].descriptorCount = 12;
	descriptorPoolSizes[1].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;

	VkDescriptorPoolCreateInfo descriptorPoolCreateInfo = {};
	descriptorPoolCreateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
	descriptorPoolCreateInfo.flags = 0;
	descriptorPoolCreateInfo.maxSets = 1;
	descriptorPoolCreateInfo.pNext = nullptr;
	descriptorPoolCreateInfo.poolSizeCount = (uint32)descriptorPoolSizes.size();
	descriptorPoolCreateInfo.pPoolSizes = descriptorPoolSizes.data();
	vkCreateDescriptorPool(m_pGraphics->GetDevice(), &descriptorPoolCreateInfo, nullptr, &m_DescriptorPool);

	//Allocate descriptor set
	VkDescriptorSetAllocateInfo descriptorAllocateInfo[1];
	descriptorAllocateInfo[0].sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
	descriptorAllocateInfo[0].descriptorPool = m_DescriptorPool;
	descriptorAllocateInfo[0].descriptorSetCount = 1;
	descriptorAllocateInfo[0].pNext = nullptr;
	descriptorAllocateInfo[0].pSetLayouts = &m_DescriptorSetLayout;
	m_DescriptorSets.resize(1);
	vkAllocateDescriptorSets(m_pGraphics->GetDevice(), descriptorAllocateInfo, m_DescriptorSets.data());

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
	vkCreateRenderPass(m_pGraphics->GetDevice(), &renderPassInfo, nullptr, &m_RenderPass);


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

	std::vector<VkPipelineShaderStageCreateInfo> shaderCreateInfos;
	for (auto& pShader : m_Shaders)
	{
		shaderCreateInfos.push_back(pShader->GetPipelineCreateInfo());
	}

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
	pipelineInfo.pStages = shaderCreateInfos.data();
	pipelineInfo.stageCount = (unsigned int)shaderCreateInfos.size();
	pipelineInfo.renderPass = m_RenderPass;
	pipelineInfo.subpass = 0;

	vkCreateGraphicsPipelines(m_pGraphics->GetDevice(), m_pGraphics->GetPipelineCache(), 1, &pipelineInfo, nullptr, &m_Pipeline);
}