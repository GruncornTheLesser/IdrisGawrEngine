#pragma once
#define GLFW_INCLUDE_VULKAN
#define VK_USE_PLATFORM_WIN32_KHR
#include <glfw3.h>
#include <string>
#include <vector>
#include <glm/glm.hpp>
#include <array>

class Application {
	// window
	struct Callback
	{
		static void glfwError(int, const char* message);
	};
	friend struct Application::Callback;

	struct SwapchainImage;
	struct Frame;

public:
	//context
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

	// application
	Application(const char* name = "GawrEngine");

	~Application();

	void run();
private:
	// context
	/* create vulkan instance*/
	void createVkInstance();
	/*selects device from available gpus*/
	void getPhysicalDevice();
	/*creates a logical device for vulkan functionality*/
	void createLogicalDevice();
	
	// window
	/*creates glfw window and surface */
	void createWindow(int width, int height, const char* title, bool fullscreen = false);
	/*creates renderpass *REQUIRES SURFACE FORMAT* */
	void createRenderPass();
	/*creates swap chain, *REQUIRES RENDERPASS* */
	void createSwapChain();
	/*creates swapchain Images *REQUIRES SWAPCHAIN* */
	void createImages();

	// shader
	/*creates command pool*/
	void createCommandPool();
	/*creates command buffer in pool *REQUIRES CMDPOOL* */
	void createFrames();

	void createGraphicsPipeline();
	
	VkShaderModule loadShader(std::string& filepath);

	uint32_t acquireImage(Frame& frame);

	void recordCommand(Frame& frame, uint32_t imageIndex);

	void submitCommand(Frame& frame, uint32_t imageIndex);

	void recreateSwapChain();

	void getQueueFamilies();

	// Application
	VkInstance					m_instance;
	VkPhysicalDevice			m_physicalDevice;
	VkDevice					m_logicalDevice;

	// window
	GLFWwindow*					m_window;
	VkSurfaceKHR				m_surface;

	VkFormat					m_surfaceFormat;
	VkExtent2D					m_surfaceExtent;

	// renderer
	VkRenderPass				m_renderPass;
	VkSwapchainKHR				m_swapchain;	
	VkQueue						m_presentQueue;
	uint32_t					m_presentQueueIndex;
	VkQueue						m_graphicsQueue;
	uint32_t					m_graphicsQueueIndex;

	struct SwapchainImage {
		VkImage			m_image;
		VkImageView		m_view;
		VkFramebuffer	m_framebuffer;
	};
	std::vector<SwapchainImage> m_swapchainImages;

	VkCommandPool				m_cmdPool;			// 1 per thread

	// Aim for 15-30 command buffers and 5-10 vkQueueSubmit() calls per frame
	struct Frame {
		VkCommandBuffer m_cmdBuffer;
		VkSemaphore		m_imageAvailable;
		VkSemaphore		m_renderFinished;
		VkFence			m_inFlight;
	};
	std::array<Frame, 2> m_frames;

	// shader program
	VkPipelineLayout			m_pipelineLayout;
	VkPipeline					m_graphicsPipeline;

};

// class Application
	// Window
		// GLFWwindow
		// Surface
		// 
	// Graphics
	
	

// Context
	// Instance
	// Device
	// PhysicalDevice
	
// Window
	// GLFW Window
	// Surface
	// Surface Format
	// Surface Extent

// RenderContext
	// RenderPass
	// Swapchain
	
	// SwapchainImages
		// VkImage
		// VkImageView
		// VkFramebuffer

	// Frame (x doule or single buffered)
		// cmdBuffer
		// fences, semaphores -> etc

// ShaderProgram
	// Pipeline Layout
	// Pipeline

// 
