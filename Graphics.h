#pragma once
class Shader;
class UniformBuffer;
class VertexBuffer;
class IndexBuffer;
class Texture2D;

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

	VkCommandBuffer GetCommandBuffer(const bool begin);
	void FlushCommandBuffer(VkCommandBuffer cmdBuffer);

	void Shutdown();

	const VkDevice& GetDevice() const { return m_Device; }
	bool MemoryTypeFromProperties(uint32_t typeBits, VkFlags requirements_mask, uint32_t* typeIndex);

private:
	void CreateDevice();
	void ConstructWindow();
	bool CreateVulkanInstance();
	void CreateSwapchain();
	void CreatePipelineCache();
	void CreateRenderPass();
	void CreateDepthStencil();
	void CreateSynchronizationPrimitives();
	void CreateCommandBuffers();
	void CreateCommandPool();
	void Gameloop();

	void UpdateUniforms();

	void BuildCommandBuffers();

	void Draw();

	HWND GetWindow() const;

	VkInstance m_Instance;
	VkDevice m_Device;
	VkQueue m_DeviceQueue;
	VkPhysicalDeviceMemoryProperties m_MemoryProperties;
	VkCommandPool m_CommandPool;
	std::vector<VkCommandBuffer> m_CommandBuffers;
	VkSurfaceKHR m_Surface;
	VkSwapchainKHR m_SwapChain;
	VkPipelineCache m_PipelineCache;

	VkSemaphore m_PresentCompleteSemaphore;
	VkSemaphore m_RenderCompleteSemaphore;
	std::vector<VkFence> m_WaitFences;

	std::vector<Texture2D*> m_SwapchainImages;
	Texture2D* m_pDepthTexture;

	VkDescriptorSetLayout m_DescriptorSetLayout;
	VkPipelineLayout m_PipelineLayout;
	VkDescriptorPool m_DescriptorPool;

	VkRenderPass m_RenderPass;
	std::vector<VkDescriptorSet> m_DescriptorSets;
	std::vector<VkFramebuffer> m_FrameBuffers;

	VkPipeline m_GraphicsPipeline;

	size_t m_CurrentBuffer = 0;
	int m_QueueFamilyIndex = -1;
	std::vector<VkQueueFamilyProperties> m_QueueFamilyProperties;
	VkPhysicalDevice m_PhysicalDevice;

	std::vector<Shader*> m_Shaders;
	SDL_Window* m_pWindow = nullptr;
	unsigned int m_WindowWidth = 1240;
	unsigned int m_WindowHeight = 720;

	UniformBuffer* m_pUniformBuffer = nullptr;
	VertexBuffer* m_pVertexBuffer = nullptr;
	IndexBuffer* m_pIndexBuffer = nullptr;
	Texture2D* m_pImageTexture = nullptr;

	glm::mat4 m_ProjectionMatrix;
	glm::mat4 m_ViewMatrix;
	glm::mat4 m_ModelMatrix;
	glm::mat4 m_MvpMatrix;

	int m_FrameCount = 0;
};

