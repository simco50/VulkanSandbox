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

	void CreateFrameBuffer();
	void CopyBufferWithStaging(VkBuffer targetBuffer, void* pData);

	std::unique_ptr<CommandBuffer> GetTempCommandBuffer(const bool begin);

	void FlushCommandBuffer(std::unique_ptr<CommandBuffer>& cmdBuffer);

	VkPipelineCache GetPipelineCache() const { return m_PipelineCache; }
	const VkPhysicalDeviceProperties& GetDeviceProperties() const { return m_DeviceProperties; };

	VkCommandPool GetCommandPool() const { return m_CommandPool; }
	DescriptorPool* GetDescriptorPool() const { return m_pDescriptorPool.get(); }

	void Shutdown();

	const VkDevice& GetDevice() const { return m_Device; }
	bool MemoryTypeFromProperties(uint32_t typeBits, VkFlags requirements_mask, uint32_t* typeIndex);

	int GetBackbufferIndex() const { return (int)m_CurrentBuffer; }
	int GetBackbufferCount() const { return (int)m_FrameBuffers.size(); }

private:
	void CreateDevice();
	void ConstructWindow();
	bool CreateVulkanInstance();
	void CreateSwapchain();
	void CreatePipelineCache();
	void CreateDescriptorPool();
	void CreateRenderPass();
	void CreateDepthStencil();
	void CreateSynchronizationPrimitives();
	void CreateCommandBuffers();
	void CreateCommandPool();
	void Gameloop();
	bool CheckValidationLayerSupport(const std::vector<const char*>& layers);

	void UpdateUniforms();

	void BuildCommandBuffers();

	void Draw();

	HWND GetWindow() const;

	std::unique_ptr<DescriptorPool> m_pDescriptorPool;

	std::unique_ptr<Material> m_pMaterial;

	VkInstance m_Instance;
	VkDevice m_Device;
	VkQueue m_DeviceQueue;
	VkPhysicalDeviceMemoryProperties m_MemoryProperties;
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

	VkRenderPass m_RenderPass;
	std::vector<VkFramebuffer> m_FrameBuffers;

	size_t m_CurrentBuffer = 0;
	int m_QueueFamilyIndex = -1;
	std::vector<VkQueueFamilyProperties> m_QueueFamilyProperties;
	VkPhysicalDevice m_PhysicalDevice;
	VkPhysicalDeviceProperties m_DeviceProperties;

	SDL_Window* m_pWindow = nullptr;
	unsigned int m_WindowWidth = 1240;
	unsigned int m_WindowHeight = 720;

	UniformBuffer* m_pUniformBuffer = nullptr;
	Texture2D* m_pImageTexture = nullptr;

	std::vector<std::unique_ptr<Drawable>> m_Drawables;

	glm::mat4 m_ProjectionMatrix;
	glm::mat4 m_ViewMatrix;
	glm::mat4 m_ModelMatrix;
	glm::mat4 m_MvpMatrix;

	int m_FrameCount = 0;
};

