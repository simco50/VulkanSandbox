#include "stdafx.h"
#include "Material.h"

#include "tinyxml2.h"
#include "Shader.h"
#include "Graphics.h"

#include "VulkanHelpers.h"
#include "DescriptorPool.h"

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
		attributeDesc.push_back(VkHelpers::VertexAttributeDescriptor::Construct(0, (unsigned int)attributeDesc.size(), offset, format));
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
	VkPipelineLayoutCreateInfo pipelineLayoutCreateInfo = VkHelpers::PipelineLayoutDescriptor();
	pipelineLayoutCreateInfo.setLayoutCount = 1;
	pipelineLayoutCreateInfo.pSetLayouts = &m_DescriptorSetLayout;
	vkCreatePipelineLayout(m_pGraphics->GetDevice(), &pipelineLayoutCreateInfo, nullptr, &m_PipelineLayout);

	//Allocate descriptor set
	m_DescriptorSets.push_back(m_pGraphics->GetDescriptorPool()->Allocate(m_DescriptorSetLayout));

	//Create render pass
	//Attachments
	std::vector<VkAttachmentDescription> attachments;
	attachments.push_back(VkHelpers::AttachmentDescriptor::ConstructColor(VK_FORMAT_B8G8R8A8_UNORM, 1));
	attachments.push_back(VkHelpers::AttachmentDescriptor::ConstructDepth(VK_FORMAT_D16_UNORM, 1));

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
	VkPipelineInputAssemblyStateCreateInfo inputAssemblyInfo = VkHelpers::PipelineInputAssemblyState();

	//Rasterization state
	VkPipelineRasterizationStateCreateInfo rasterizerStateInfo = VkHelpers::PipelineRasterizationState();

	//Blend state
	VkPipelineColorBlendStateCreateInfo blendStateInfo = VkHelpers::BlendState();
	VkPipelineColorBlendAttachmentState attState = VkHelpers::BlendAttachmentState();
	blendStateInfo.attachmentCount = 1;
	blendStateInfo.pAttachments = &attState;

	//Viewport state
	VkPipelineViewportStateCreateInfo viewportStateInfo = VkHelpers::ViewportDescriptor();
	viewportStateInfo.viewportCount = 1;
	dynamicStateEnables[dynamicState.dynamicStateCount++] = VK_DYNAMIC_STATE_VIEWPORT;
	viewportStateInfo.scissorCount = 1;
	VkRect2D scissor;
	scissor.offset.x = 0;
	scissor.offset.y = 0;
	scissor.extent.width = 1240;
	scissor.extent.height = 720;
	viewportStateInfo.pScissors = &scissor;
	viewportStateInfo.pViewports = nullptr;

	//Depth stencil state
	VkPipelineDepthStencilStateCreateInfo depthStencilStateInfo = VkHelpers::DepthStencilState();

	//MSAA state
	VkPipelineMultisampleStateCreateInfo multiSampleStateInfo = VkHelpers::MultisampleState();

	std::vector<VkPipelineShaderStageCreateInfo> shaderCreateInfos;
	for (auto& pShader : m_Shaders)
	{
		shaderCreateInfos.push_back(VkHelpers::ShaderCreateInfo::Construct("main", pShader->GetStage(), pShader->GetShaderObject()));
	}

	//Graphics pipeline
	VkGraphicsPipelineCreateInfo pipelineInfo = VkHelpers::GraphicsPipelineDescriptor();
	pipelineInfo.layout = m_PipelineLayout;
	pipelineInfo.pVertexInputState = &vertexStateInfo;
	pipelineInfo.pInputAssemblyState = &inputAssemblyInfo;
	pipelineInfo.pRasterizationState = &rasterizerStateInfo;
	pipelineInfo.pColorBlendState = &blendStateInfo;
	pipelineInfo.pMultisampleState = &multiSampleStateInfo;
	pipelineInfo.pDynamicState = &dynamicState;
	pipelineInfo.pViewportState = &viewportStateInfo;
	pipelineInfo.pDepthStencilState = &depthStencilStateInfo;
	pipelineInfo.pStages = shaderCreateInfos.data();
	pipelineInfo.stageCount = (unsigned int)shaderCreateInfos.size();
	pipelineInfo.renderPass = m_RenderPass;

	vkCreateGraphicsPipelines(m_pGraphics->GetDevice(), m_pGraphics->GetPipelineCache(), 1, &pipelineInfo, nullptr, &m_Pipeline);
}