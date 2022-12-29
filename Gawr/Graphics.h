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

	void recordCommandBuffer(VkCommandBuffer cmdBuffer, uint32_t imageIndex);




	void prepareFrame(uint32_t currentFrame);

	void submitFrame(uint32_t currentFrame);

	void mainloop();



	void recreateSwapChain();

	/*gets the index of the queue family which matches flag*/
	uint32_t getQueueFamilyIndex(VkQueueFlagBits flag);
	/*gets the index of queue family which supports presentation(ie rendering to the screen)*/
	uint32_t getQueueFamilyIndexPresent();

	VkInstance					m_instance;
	VkPhysicalDevice			m_physicalDevice;
	VkDevice					m_logicalDevice;

	GLFWwindow*					m_window;
	VkSurfaceKHR				m_surface;

	VkRenderPass				m_renderPass;

	VkSwapchainKHR				m_swapChain;
	VkExtent2D					m_swapChainExtent;
	VkPresentModeKHR			m_swapChainPresentMode;
	VkFormat					m_surfaceFormat;

	struct SwapchainImage {
		VkImage					m_image;
		VkImageView				m_view;
		VkFramebuffer			m_framebuffer;
	};
	std::vector<SwapchainImage> m_swapChainImages;

	VkQueue						m_presentQueue;
	VkQueue						m_graphicsQueue;

	VkCommandPool				m_cmdPool;			// 1 per thread

	// Aim for 15-30 command buffers and 5-10 vkQueueSubmit() calls per frame
	struct Renderer {
		VkCommandBuffer m_cmdBuffer;
		VkSemaphore		m_imageAvailable;
		VkSemaphore		m_renderFinished;
		VkFence			m_inFlight;
	};
	std::array<Renderer, 2>		m_frames;

	// shader program
	VkPipelineLayout			m_pipelineLayout;
	VkPipeline					m_graphicsPipeline;
	
	bool						m_resized = false;	
};

