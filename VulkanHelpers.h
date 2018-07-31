#pragma once

namespace VkHelpers
{
	struct PipelineInputAssemblyState
	{
		constexpr inline operator VkPipelineInputAssemblyStateCreateInfo() const
		{
			VkPipelineInputAssemblyStateCreateInfo inputAssemblyInfo = {};
			inputAssemblyInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
			inputAssemblyInfo.pNext = nullptr;
			inputAssemblyInfo.flags = 0;
			inputAssemblyInfo.primitiveRestartEnable = VK_FALSE;
			inputAssemblyInfo.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
			return inputAssemblyInfo;
		}
	};

	struct PipelineRasterizationState
	{
		constexpr inline operator VkPipelineRasterizationStateCreateInfo() const
		{
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
			return rasterizerStateInfo;
		}
	};

	struct BlendState
	{
		inline operator VkPipelineColorBlendStateCreateInfo() const
		{
			VkPipelineColorBlendStateCreateInfo blendStateInfo = {};
			blendStateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
			blendStateInfo.pNext = nullptr;
			blendStateInfo.flags = 0;
			blendStateInfo.logicOpEnable = VK_FALSE;
			blendStateInfo.logicOp = VK_LOGIC_OP_NO_OP;
			blendStateInfo.blendConstants[0] = 1.0f;
			blendStateInfo.blendConstants[1] = 1.0f;
			blendStateInfo.blendConstants[2] = 1.0f;
			blendStateInfo.blendConstants[3] = 1.0f;
			return blendStateInfo;
		}
	};

	struct BlendAttachmentState
	{
		inline operator VkPipelineColorBlendAttachmentState() const
		{
			VkPipelineColorBlendAttachmentState attState = {};
			attState.colorWriteMask = 0xf;
			attState.blendEnable = VK_FALSE;
			attState.alphaBlendOp = VK_BLEND_OP_ADD;
			attState.colorBlendOp = VK_BLEND_OP_ADD;
			attState.srcColorBlendFactor = VK_BLEND_FACTOR_ZERO;
			attState.dstColorBlendFactor = VK_BLEND_FACTOR_ZERO;
			attState.srcAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
			attState.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
			return attState;
		}
	};

	struct DepthStencilState
	{
		constexpr inline operator VkPipelineDepthStencilStateCreateInfo() const
		{
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
			return depthStencilStateInfo;
		}
	};

	struct ViewportDescriptor
	{
		constexpr inline operator VkPipelineViewportStateCreateInfo() const
		{
			VkPipelineViewportStateCreateInfo viewportStateInfo = {};
			viewportStateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
			viewportStateInfo.pNext = nullptr;
			viewportStateInfo.flags = 0;
			viewportStateInfo.viewportCount = 0;
			viewportStateInfo.scissorCount = 0;
			viewportStateInfo.pScissors = nullptr;
			viewportStateInfo.pViewports = nullptr;
			return viewportStateInfo;
		}
	};

	struct MultisampleState
	{
		constexpr inline operator VkPipelineMultisampleStateCreateInfo() const
		{
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
			return multiSampleStateInfo;
		}
	};

	struct PipelineLayoutDescriptor
	{
		constexpr inline operator VkPipelineLayoutCreateInfo() const
		{
			VkPipelineLayoutCreateInfo pipelineLayoutCreateInfo = {};
			pipelineLayoutCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
			pipelineLayoutCreateInfo.flags = 0;
			pipelineLayoutCreateInfo.pNext = nullptr;
			pipelineLayoutCreateInfo.pushConstantRangeCount = 0;
			pipelineLayoutCreateInfo.pPushConstantRanges = nullptr;
			return pipelineLayoutCreateInfo;
		}
	};

	struct VertexAttributeDescriptor
	{
		constexpr static inline VkVertexInputAttributeDescription Construct(const unsigned int binding, const unsigned int location, const unsigned int offset, const VkFormat format)
		{
			VkVertexInputAttributeDescription desc = {};
			desc.binding = binding;
			desc.location = location;
			desc.offset = offset;
			desc.format = format;
			return desc;
		}
	};

	struct GraphicsPipelineDescriptor
	{
		constexpr inline operator VkGraphicsPipelineCreateInfo() const
		{
			VkGraphicsPipelineCreateInfo pipelineInfo = {};
			pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
			pipelineInfo.pNext = nullptr;
			pipelineInfo.basePipelineHandle = VK_NULL_HANDLE;
			pipelineInfo.basePipelineIndex = 0;
			pipelineInfo.flags = 0;
			pipelineInfo.subpass = 0;
			return pipelineInfo;
		}
	};

	struct ShaderCreateInfo
	{
		constexpr static inline VkPipelineShaderStageCreateInfo Construct(const char* name, const VkShaderStageFlagBits stage, const VkShaderModule module)
		{
			VkPipelineShaderStageCreateInfo shaderStageCreateInfo = {};
			shaderStageCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
			shaderStageCreateInfo.flags = 0;
			shaderStageCreateInfo.pNext = nullptr;
			shaderStageCreateInfo.pSpecializationInfo = nullptr;
			shaderStageCreateInfo.pName = name;
			shaderStageCreateInfo.module = module;
			shaderStageCreateInfo.stage = stage;
			return shaderStageCreateInfo;
		}
	};

	struct AttachmentDescriptor
	{
		constexpr static inline VkAttachmentDescription ConstructDepth(VkFormat format, int msaa)
		{
			VkAttachmentDescription attachment = {};
			attachment.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
			attachment.flags = 0;
			attachment.format = format;
			attachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
			attachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
			attachment.samples = (VkSampleCountFlagBits)msaa;
			attachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
			attachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
			attachment.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
			return attachment;
		}
		constexpr static inline VkAttachmentDescription ConstructColor(VkFormat format, int msaa)
		{
			VkAttachmentDescription attachment = {};
			attachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
			attachment.flags = 0;
			attachment.format = format;
			attachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
			attachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
			attachment.samples = (VkSampleCountFlagBits)msaa;
			attachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
			attachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
			attachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
			return attachment;
		}
	};
}