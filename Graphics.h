#pragma once
#include "CommandBuffer.h"
class Shader;
class UniformBuffer;
class VertexBuffer;
class IndexBuffer;
class Texture2D;
class Drawable;
class Material;
class DescriptorPool;
class VulkanAllocator;

enum class DescriptorGroup
{
	Frame = 0,
	View,
	Object,
	Material,
	MAX
};

enum class DescriptorBinding
{
	DiffuseTexture = 1,
	ModelMatrices = 0,
	FrameData = 2,
};

struct SampleVertex
{
	glm::vec3 Position;
	glm::vec3 Normal;
	glm::vec2 TexCoord;
};

class Graphics
{
public:
	Graphics();
	~Graphics();

	void Initialize();

	void CopyBufferWithStaging(VkBuffer targetBuffer, void* pData);

	std::unique_ptr<CommandBuffer> GetTempCommandBuffer(const bool begin);
	VkRenderPass GetRenderPass() const { return m_RenderPass; }

	void FlushCommandBuffer(std::unique_ptr<CommandBuffer>& cmdBuffer);

	VkPipelineCache GetPipelineCache() const { return m_PipelineCache; }
	const VkPhysicalDeviceProperties& GetDeviceProperties() const { return m_DeviceProperties; };

	VkCommandPool GetCommandPool() const { return m_CommandPool; }
	DescriptorPool* GetDescriptorPool() const { return m_pDescriptorPool.get(); }
	VulkanAllocator* GetAllocator() const { return m_pAllocator; }

	void Shutdown();

	const VkDevice& GetDevice() const { return m_Device; }

	int GetBackbufferIndex() const { return (int)m_CurrentBuffer; }
	int GetBackbufferCount() const { return (int)m_FrameBuffers.size(); }

	VkDescriptorSet GetDestriptorSet(DescriptorGroup group);
	VkPipelineLayout GetPipelineLayout() const { return m_PipelineLayout; }

private:
	void ConstructWindow();
	bool CreateVulkanInstance();
	void CreateDevice(VkInstance instance);
	void CreateSwapchain();
	void CreateCommandPool();
	void CreateCommandBuffers();
	void CreateSynchronizationPrimitives();
	void CreateDescriptorPool();
	void CreatePipelineCache();
	void UnloadPipelineCache();
	void CreateRenderPassAndFrameBuffer();
	void CreateGlobalPipelineLayout();

	void BuildCommandBuffers();

	void UpdateUniforms();
	void Gameloop();
	void Draw();

	bool CheckValidationLayerSupport(const std::vector<const char*>& layers);
	HWND GetWindow() const;

	std::unique_ptr<DescriptorPool> m_pDescriptorPool;
	VulkanAllocator* m_pAllocator;

	std::unique_ptr<Material> m_pMaterial;

	VkInstance m_Instance;

	VkDevice m_Device;
	VkPhysicalDevice m_PhysicalDevice;
	int m_QueueFamilyIndex = -1;
	std::vector<VkQueueFamilyProperties> m_QueueFamilyProperties;
	VkPhysicalDeviceProperties m_DeviceProperties;
	VkQueue m_DeviceQueue;

	VkCommandPool m_CommandPool;
	std::vector<std::unique_ptr<CommandBuffer>> m_CommandBuffers;

	VkSurfaceKHR m_Surface;
	VkSwapchainKHR m_SwapChain;
	VkPipelineCache m_PipelineCache;

	VkSemaphore m_PresentCompleteSemaphore;
	VkSemaphore m_RenderCompleteSemaphore;
	std::vector<VkFence> m_WaitFences;

	std::vector<Texture2D*> m_SwapchainImages;
	Texture2D* m_pDepthTexture;

	VkDescriptorSet m_ObjectDescriptorSet;
	VkDescriptorSet m_FrameDescriptorSet;
	VkPipelineLayout m_PipelineLayout;
	std::vector<VkDescriptorSetLayout> m_DescriptorSetLayouts;

	size_t m_CurrentBuffer = 0;
	std::vector<VkFramebuffer> m_FrameBuffers;
	VkRenderPass m_RenderPass;

	SDL_Window* m_pWindow = nullptr;
	unsigned int m_WindowWidth = 1240;
	unsigned int m_WindowHeight = 720;

	UniformBuffer* m_pUniformBuffer = nullptr;
	UniformBuffer* m_pUniformBufferPerFrame = nullptr;

	std::vector<std::unique_ptr<Drawable>> m_Drawables;

	glm::mat4 m_ProjectionMatrix;
	glm::mat4 m_ViewMatrix;

	int m_FrameCount = 0;
};

