#pragma once
#define GLFW_INCLUDE_VULKAN
#define VK_USE_PLATFORM_WIN32_KHR
#include <glfw3.h>
#include <string>
#include <vector>
#include <glm/glm.hpp>
#include <array>

class Application {
	struct Callback
	{
		static void glfwError(int, const char* message);
		static void glfwWindowResize(GLFWwindow* wnd, int width, int height);
	};

	friend struct Application::Callback;
public:
	const std::vector<const char*> instanceExtensions = {
	#if defined(_DEBUG)
		VK_EXT_DEBUG_UTILS_EXTENSION_NAME
	#endif
	};

	const std::vector<const char*> deviceExtensions = {
		VK_KHR_SWAPCHAIN_EXTENSION_NAME
	};

	const std::vector<const char*> validationLayers = {
	#if defined(_DEBUG)
		"VK_LAYER_KHRONOS_validation"
	#endif
	};

	Application(const char* name = "GawrEngine");

	~Application();

private:
	/* create vulkan instance*/
	void createVkInstance();
	/*selects device from available gpus*/
	void getPhysicalDevice();
	/*creates a logical device for vulkan functionality*/
	void createLogicalDevice();
	/*creates glfw window*/
	void createWindow(int width, int height, const char* title);
	/*gets surface from glfw window*/
	void createSurface();
	/*creates swap chain*/
	void createSwapChain();

	void createFramebuffer();

	/*creates command pool*/
	void createCommandPool();
	/*creates command buffer in pool*/
	void createFrames();



	VkShaderModule loadShader(std::string& filepath);

	void createGraphicsPipeline();
	
	void recordCommandBuffer(VkCommandBuffer cmdBuffer, uint32_t imageIndex);

	void mainloop();

	void recreateSwapChain();

	void cleanupSwapChain();

	/*gets the index of the queue family which matches flag*/
	uint32_t getQueueFamilyIndex(VkQueueFlagBits flag);
	/*gets the index of queue family which supports presentation(ie rendering to the screen)*/
	uint32_t getQueueFamilyIndexPresent();

	VkInstance					m_instance;
	VkDevice					m_logicalDevice;
	VkPhysicalDevice			m_physicalDevice;

	GLFWwindow*					m_window;

	VkSurfaceKHR				m_surface;
	VkSwapchainKHR				m_swapChain;
	VkFormat					m_surfaceFormat;

	VkExtent2D					m_swapChainExtent;
	VkPresentModeKHR			m_swapChainPresentMode;
	
	/*
	struct SwapchainImage {
		VkImage m_image;
		VkImageView m_view;
		VkFramebuffer m_framebuffer;
	};
	*/
	std::vector<VkImage>		m_swapChainImages;
	std::vector<VkImageView>	m_swapChainImageViews;
	std::vector<VkFramebuffer>	m_swapChainFramebuffers;
	bool						m_resized = false;
	VkRenderPass				m_renderPass;

	VkQueue						m_graphicsQueue;
	VkCommandPool				m_cmdPool;
	
	VkQueue						m_presentQueue;
	//VkCommandPool				m_cmdPool;

	// shader program
	VkPipelineLayout			m_pipelineLayout;
	VkPipeline					m_graphicsPipeline;
	
	// rendering
	struct Renderer {
		VkCommandBuffer m_cmdBuffer;
		VkSemaphore m_imageAvailable;
		VkSemaphore m_renderFinished;
		VkFence m_inFlight;
	};

	uint32_t m_currentFrame = 0;
	std::array<Renderer, 2> m_frames;
};

