#pragma once
#define GLFW_INCLUDE_VULKAN
#define VK_USE_PLATFORM_WIN32_KHR
#include <glfw3.h>
#include <string>
#include <vector>




class Graphics {
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
	void createSwapChainImages();
	


	void createRenderPass();

	void createFramebuffer();

	VkShaderModule loadShader(std::string& filepath);
	void createPipeline(std::string& vertShader, std::string& fragShader);

	

	/*creates command pool*/
	void createCommandPool();
	/*creates command buffer in pool*/
	void createCommandBuffer();
	
	void recordCommandBuffer(VkCommandBuffer cmdBuffer, uint32_t imageIndex);

	void createSyncObjects();

	void mainloop();
	void drawFrame();

	/*gets the index of the queue family which matches flag*/
	uint32_t getQueueFamilyIndex(VkQueueFlagBits flag);
	uint32_t getQueueFamilyIndexPresent();

	VkInstance					m_instance;
	VkDevice					m_logicalDevice;
	VkPhysicalDevice			m_physicalDevice;

	VkQueue						m_graphicsQueue;
	VkQueue						m_presentQueue;

	VkSurfaceKHR				m_surface;
	VkSwapchainKHR				m_swapChain;
	VkExtent2D					m_swapChainExtent;
	VkFormat					m_swapChainFormat;
	VkPresentModeKHR			m_swapChainPresentMode;
	std::vector<VkImage>		m_swapChainImages;
	std::vector<VkImageView>	m_swapChainImageViews;


	std::vector<VkFramebuffer>	m_swapChainFramebuffers;
	VkRenderPass				m_renderPass;
	
	VkPipelineLayout			m_pipelineLayout;
	VkPipeline					m_graphicsPipeline;
	VkCommandPool				m_cmdPool;
	VkCommandBuffer				m_cmdBuffer;

	VkSemaphore					m_imageAvailableSemaphore;
	VkSemaphore					m_renderFinishedSemaphore;
	VkFence						m_inFlightFence;

	GLFWwindow*					m_window;
	const char*					m_title;
};

namespace Gawr {
	namespace Core {
		class Window {


		private:
			VkSurfaceKHR	m_surface;
			VkSwapchainKHR	m_swapChain;
			GLFWwindow*		m_window;

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
	}
}


