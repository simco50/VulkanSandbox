#include "stdafx.h"
#include "Material.h"

#include "tinyxml2.h"
#include "Shader.h"
#include "Graphics.h"

#include "VulkanHelpers.h"
#include "DescriptorPool.h"
#include "Texture2D.h"

Material::Material(Graphics* pGraphics) :
	m_pGraphics(pGraphics)
{

}

Material::~Material()
{
	m_Textures.clear();
	vkDestroyPipeline(m_pGraphics->GetDevice(), m_Pipeline, nullptr);
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
	else if (type == "mat44")
	{
		format = VK_FORMAT_UNDEFINED;
		size = sizeof(glm::mat4);
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

	pCurrent = pRootNode->FirstChildElement("Resources");
	XML::XMLElement* pResource = pCurrent->FirstChildElement();
	while (pResource)
	{
		if (strcmp(pResource->Value(), "Texture2D") == 0)
		{
			std::string binding = pResource->Attribute("binding");
			std::string source = pResource->Attribute("source");
			std::unique_ptr<Texture2D> pTexture = std::make_unique<Texture2D>(m_pGraphics);
			pTexture->Load(source);

			if (binding == "Diffuse")
			{
				m_Textures[(int)DescriptorBinding::DiffuseTexture] = std::move(pTexture);
			}
		}
		pResource = pResource->NextSiblingElement();
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
	pipelineInfo.layout = m_pGraphics->GetPipelineLayout();
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
	pipelineInfo.renderPass = m_pGraphics->GetRenderPass();

	vkCreateGraphicsPipelines(m_pGraphics->GetDevice(), m_pGraphics->GetPipelineCache(), 1, &pipelineInfo, nullptr, &m_Pipeline);
	
	m_DescriptorSet = m_pGraphics->GetDestriptorSet(DescriptorGroup::Material);

	std::vector<VkWriteDescriptorSet> writes;
	for (const auto& tex : m_Textures)
	{
		VkDescriptorImageInfo texInfo;
		texInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		texInfo.imageView = (VkImageView)tex.second->GetView();
		texInfo.sampler = (VkSampler)tex.second->GetSampler();

		VkWriteDescriptorSet write = {};
		write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		write.descriptorCount = 1;
		write.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		write.dstBinding = tex.first;
		write.dstSet = m_DescriptorSet;
		write.pImageInfo = &texInfo;
		write.pNext = nullptr;
		write.dstArrayElement = 0;

		writes.push_back(write);
	}
	vkUpdateDescriptorSets(m_pGraphics->GetDevice(), writes.size(), writes.data(), 0, nullptr);
}