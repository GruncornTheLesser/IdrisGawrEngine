#pragma once
#define GLFW_INCLUDE_VULKAN
#define VK_USE_PLATFORM_WIN32_KHR
#include <glfw3.h>
#include <string>
#include <vector>



class Graphics {
	friend struct callback;
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

	const std::vector<VkQueueFlagBits> requiredQueueFamilies = {
		VK_QUEUE_GRAPHICS_BIT,
	};

	Graphics(const char* title = "GawrEngine");
	~Graphics();

private:
	/*checks vulkan supports instance validation layers requested*/
	void queryInstanceValidationLayer();
	/*checks vulkan supports instance extensions requested (set with 'instanceExtensions')*/
	void queryInstanceExtensions();
	/* create vulkan instance*/
	void createInstance();
	/*selects device from available gpus*/
	void getPhysicalDevice();
	/*checks physical device supports validation layers requested (set with 'validationLayers')*/
	void queryPhysicalDeviceValidationLayer();
	/*checks physical device supports extensions requested (set with 'deviceExtensions')*/
	void queryPhysicalDeviceExtensions();
	/*creates a logical device for vulkan functionality*/
	void createLogicalDevice();
	/*creates glfw window*/
	void createWindow(int width, int height);
	/*gets surface from glfw window*/
	void createSurface();
	/*creates swap chain*/
	void createSwapChain();
	/*retrieves swap chain images and image views*/
	void createImageViews();

	void createRenderPass();

	void createFramebuffer();

	VkShaderModule loadShader(std::string& filepath);

	void createGraphicsPipeline();
	/*creates command pool*/
	void createCommandPool();
	/*creates command buffer in pool*/
	void createCommandBuffer();
	
	void recordCommandBuffer(VkCommandBuffer cmdBuffer, uint32_t imageIndex);

	void createSyncObjects();

	void mainloop();

	void drawFrame();

	void recreateSwapChain();

	void cleanupSwapChain();

	/*gets the index of the queue family which matches flag*/
	uint32_t getQueueFamilyIndex(VkQueueFlagBits flag);
	/*gets the index of queue family which supports presentation(ie rendering to the screen)*/
	uint32_t getQueueFamilyIndexPresent();

	VkInstance					m_instance;
	VkDevice					m_logicalDevice;
	VkPhysicalDevice			m_physicalDevice;

	GLFWwindow* m_window;
	const char* m_title;

	VkSurfaceKHR				m_surface;
	VkSwapchainKHR				m_swapChain;
	VkExtent2D					m_swapChainExtent;
	VkFormat					m_swapChainFormat;
	VkPresentModeKHR			m_swapChainPresentMode;
	std::vector<VkImage>		m_swapChainImages;
	std::vector<VkImageView>	m_swapChainImageViews;

	VkQueue						m_graphicsQueue;
	VkQueue						m_presentQueue;

	bool						m_framebufferResized = false;
	std::vector<VkFramebuffer>	m_swapChainFramebuffers; // one framebuffer per image

	VkRenderPass				m_renderPass;
	VkPipelineLayout			m_pipelineLayout;
	VkPipeline					m_graphicsPipeline;
	VkCommandPool				m_cmdPool;
	
	uint32_t					m_currentFrame = 0;

	// per frame variables frame
	std::vector<VkCommandBuffer>m_cmdBuffers;
	std::vector<VkSemaphore>    m_imageAvailableSemaphore;
	std::vector<VkSemaphore>    m_renderFinishedSemaphore;
	std::vector<VkFence>		m_inFlightFence;

};

namespace Gawr {
	namespace Core {
		class Window {
		private:
			VkSurfaceKHR	m_surface;
			VkSwapchainKHR	m_swapChain;
			GLFWwindow*		m_glfwWindow;

			VkCommandPool				m_cmdPool;
			std::vector<VkImage>		m_swapChainImages;
			std::vector<VkImageView>	m_swapChainImageViews;
		};

		class Context {
			struct Requirements {
				VkPhysicalDeviceFeatures m_features;
				std::vector<const char*> m_instanceExtensions;
				std::vector<const char*> m_deviceExtensions;
			};

			Context();
			~Context();
			
		private:
			VkInstance		 m_instance;
			VkPhysicalDevice m_physicalDevice;
			VkDevice		 m_logicalDevice;
		};

		class Swapchain {
			VkSwapchainKHR				m_swapChain;
			VkExtent2D					m_swapChainExtent;
			VkFormat					m_swapChainFormat;
			VkPresentModeKHR			m_swapChainPresentMode;
			std::vector<VkImage>		m_swapChainImages;
			std::vector<VkImageView>	m_swapChainImageViews;
		};

		class RenderFrame { 
			VkCommandBuffer	m_cmdBuffers;
			VkSemaphore		m_imageAvailableSemaphore;
			VkSemaphore		m_renderFinishedSemaphore;
			VkFence			m_inFlightFence;
		};

		template<typename RenderFrame_t> 
			requires std::is_base_of_v<RenderFrame, RenderFrame_t>
		class Renderpass {
			static const uint32_t		MaxFramesInFlight = 2;
			uint32_t					m_currentFrame = 0;
			VkRenderPass				m_renderPass;
			VkCommandPool				m_cmdPool;
			std::vector<VkFramebuffer>	m_swapChainFramebuffers;
			RenderFrame_t				m_frames[MaxFramesInFlight]; // current frames in flight
		};

		class ShaderProgram {
			VkPipelineLayout			m_pipelineLayout;
			VkPipeline					m_graphicsPipeline;
		};
	}
}


