#include "Graphics.h"

#define GLFW_EXPOSE_NATIVE_WIN32
#include <glfw3native.h>

#include <vulkan/vk_enum_string_helper.h>

#include <stdexcept>
#include <algorithm>
#include <iterator>
#include <iostream>
#include <fstream>
#include <assert.h>
#include <set>

#ifdef _DEBUG
#define VK_ASSERT(x) {								\
	VkResult res = x; if (res != VK_SUCCESS) {		\
		std::cerr << "[Vulkan Error]" << "line : "	\
		<< __LINE__ << " - "						\
		<< string_VkResult(res) << std::endl;		\
		throw std::exception();						\
		}											\
	}						
		
		
#else
#define VK_CHECK(x) x
#endif

#define DEFAULT_GPU 1
#define MAX_FRAMES_IN_FLIGHT 1

struct callback
{
	static void frameBufferSizeCallback(GLFWwindow* wnd, int width, int height) {
		auto app = reinterpret_cast<Graphics*>(glfwGetWindowUserPointer(wnd));
		app->m_framebufferResized = true;
	}
};


Graphics::Graphics(const char* title) : m_title(title)
{
	assert(glfwInit() == GLFW_TRUE); // glfw initiated first so createInstance() can get glfw required extensions

	// create Instance
	queryInstanceValidationLayer();
	queryInstanceExtensions();
	createInstance();

	// create device
	getPhysicalDevice();					// enumerate gpus on the system
	queryPhysicalDeviceValidationLayer();	// check device validation layers
	queryPhysicalDeviceExtensions();		// check device extensions
	createLogicalDevice();					// create device object

	// create presentation swapchain
	createWindow(800, 600);	// create a native empty window
	createSurface();		// create surface (wsi extension VK_KHR_SURFACE)
	createSwapChain();		// create swapchain 
	createImageViews();// create swapchain image views

	// get queues for render pass
	vkGetDeviceQueue(m_logicalDevice, getQueueFamilyIndex(VK_QUEUE_GRAPHICS_BIT), 0, &m_graphicsQueue);
	vkGetDeviceQueue(m_logicalDevice, getQueueFamilyIndexPresent(), 0, &m_presentQueue);
	
	createRenderPass();
	createFramebuffer();

	createCommandPool();
	createCommandBuffer();

	createGraphicsPipeline();
	createSyncObjects();

	mainloop();
}

Graphics::~Graphics()
{
	for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
		vkDestroySemaphore(m_logicalDevice, m_imageAvailableSemaphore[i], nullptr);
		vkDestroySemaphore(m_logicalDevice, m_renderFinishedSemaphore[i], nullptr);
		vkDestroyFence(m_logicalDevice, m_inFlightFence[i], nullptr);
	}

	vkDestroyCommandPool(m_logicalDevice, m_cmdPool, nullptr);
	
	vkDestroyPipeline(m_logicalDevice, m_graphicsPipeline, nullptr);
	vkDestroyPipelineLayout(m_logicalDevice, m_pipelineLayout, nullptr);
	
	for (auto framebuffer : m_swapChainFramebuffers)
		vkDestroyFramebuffer(m_logicalDevice, framebuffer, nullptr);

	vkDestroyRenderPass(m_logicalDevice, m_renderPass, nullptr);

	for (auto& imageView : m_swapChainImageViews)
		vkDestroyImageView(m_logicalDevice, imageView, nullptr);
	vkDestroySwapchainKHR(m_logicalDevice, m_swapChain, nullptr);
	
	vkDestroySurfaceKHR(m_instance, m_surface, nullptr);
	glfwDestroyWindow(m_window);

	vkDestroyDevice(m_logicalDevice, nullptr);
	vkDestroyInstance(m_instance, nullptr);
}

void Graphics::queryInstanceValidationLayer()
{
// requires:
//		validation layers

#if defined(_DEBUG)
	// check validation layer support
	uint32_t layerCount;
	VK_ASSERT(vkEnumerateInstanceLayerProperties(&layerCount, nullptr));

	std::vector<VkLayerProperties> layerProperties(layerCount);
	VK_ASSERT(vkEnumerateInstanceLayerProperties(&layerCount, layerProperties.data()));

	for (const char* validationLayerName : validationLayers)
	{
		auto propertyMatch = [validationLayerName](VkLayerProperties p) { return strcmp(validationLayerName, p.layerName) == 0; };
		auto it = std::find_if(layerProperties.begin(), layerProperties.end(), propertyMatch);

		if (it == layerProperties.end())
			throw std::runtime_error("validation layer requested but not available");
	}
#endif
}

void Graphics::queryInstanceExtensions()
{
// requires:
//		instance extensions

	uint32_t extensionCount;
	VK_ASSERT(vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, nullptr));

	std::vector<VkExtensionProperties> availableExtensions(extensionCount);
	VK_ASSERT(vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, availableExtensions.data()));

	for (const char* extensionName : instanceExtensions) {
		auto extensionMatch = [extensionName](VkExtensionProperties p) { return strcmp(extensionName, p.extensionName) == 0; };
		auto it = std::find_if(availableExtensions.begin(), availableExtensions.end(), extensionMatch);

		if (it == availableExtensions.end())
			throw std::runtime_error("gpu doesnt support required extensions");
	}
}

void Graphics::createInstance()
{
// requires:
//		instance extensions
//		validation layers
//		m_title
//		m_instance

	uint32_t glfwExtensionCount;
	const char** glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);
	
	std::vector<const char*> extensions(glfwExtensionCount + instanceExtensions.size());

	std::set_union(glfwExtensions, glfwExtensions + glfwExtensionCount, instanceExtensions.begin(), instanceExtensions.end(), extensions.begin());

	// get vulkan extensions required for glfw
	VkApplicationInfo appInfo{};
	{
		appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
		appInfo.pApplicationName = m_title;
		appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
		appInfo.pEngineName = "GawrEngine";
		appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
		appInfo.apiVersion = VK_API_VERSION_1_0;
	}
	
	VkInstanceCreateInfo instanceInfo{};
	{
		instanceInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
		instanceInfo.pApplicationInfo = &appInfo;

		instanceInfo.enabledExtensionCount = static_cast<uint32_t>(extensions.size());
		instanceInfo.ppEnabledExtensionNames = extensions.data();
		
		instanceInfo.enabledLayerCount = static_cast<uint32_t>(validationLayers.size());
		instanceInfo.ppEnabledLayerNames = validationLayers.data();
	}

	VK_ASSERT(vkCreateInstance(&instanceInfo, nullptr, &m_instance));
}

void Graphics::getPhysicalDevice()
{
	// requires:
	//		m_instance
	//		m_physical device
	uint32_t deviceCount = 0;
	VK_ASSERT(vkEnumeratePhysicalDevices(m_instance, &deviceCount, nullptr));

	if (deviceCount == 0)
		throw std::runtime_error("failed to find GPU with vulkan support");

	std::vector<VkPhysicalDevice> devices(deviceCount);
	VK_ASSERT(vkEnumeratePhysicalDevices(m_instance, &deviceCount, devices.data()));

	// TODO: get device choice from setting.json
	m_physicalDevice = devices[DEFAULT_GPU];
}

void Graphics::queryPhysicalDeviceValidationLayer()
{
// requires:
//		m_physicalDevice
//		validation layers
#if defined(_DEBUG)
	// check validation layer support
	uint32_t layerCount;
	VK_ASSERT(vkEnumerateDeviceLayerProperties(m_physicalDevice, &layerCount, nullptr));

	std::vector<VkLayerProperties> layerProperties(layerCount);
	VK_ASSERT(vkEnumerateDeviceLayerProperties(m_physicalDevice, &layerCount, layerProperties.data()));

	for (const char* validationLayerName : validationLayers)
	{
		auto propertyMatch = [validationLayerName](VkLayerProperties p) { return strcmp(validationLayerName, p.layerName) == 0; };
		auto it = std::find_if(layerProperties.begin(), layerProperties.end(), propertyMatch);

		if (it == layerProperties.end())
			throw std::runtime_error("validation layer requested but not available on gpu");
	}
#endif
}

void Graphics::queryPhysicalDeviceExtensions()
{
// requires:
//		m_physicalDevice
//		deviceExtensions
	uint32_t extensionCount;
	VK_ASSERT(vkEnumerateDeviceExtensionProperties(m_physicalDevice, nullptr, &extensionCount, nullptr));

	std::vector<VkExtensionProperties> availableExtensions(extensionCount);
	VK_ASSERT(vkEnumerateDeviceExtensionProperties(m_physicalDevice, nullptr, &extensionCount, availableExtensions.data()));

	for (const char* extensionName : deviceExtensions) {
		auto extensionMatch = [extensionName](VkExtensionProperties p) { return strcmp(extensionName, p.extensionName) == 0; };
		auto it = std::find_if(availableExtensions.begin(), availableExtensions.end(), extensionMatch);

		if (it == availableExtensions.end())
			throw std::runtime_error("gpu doesnt support required extension");
	}
}

void Graphics::createLogicalDevice()
{
// requires:
//		m_physicalDevice
//		deviceExtensions
//		validationLayers
//		m_logical_device
	VkPhysicalDeviceFeatures deviceFeatures{};
	{
		vkGetPhysicalDeviceFeatures(m_physicalDevice, &deviceFeatures);
	}

	VkDeviceQueueCreateInfo queueInfo{};
	{
		float queuePriorities[1] = { 1.0 };

		queueInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
		queueInfo.queueFamilyIndex = getQueueFamilyIndex(VK_QUEUE_GRAPHICS_BIT);
		queueInfo.queueCount = 1;
		queueInfo.pQueuePriorities = queuePriorities;
		queueInfo.pNext = NULL;
	}

	VkDeviceCreateInfo deviceInfo{};
	{
		deviceInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
		deviceInfo.pQueueCreateInfos = &queueInfo;
		deviceInfo.queueCreateInfoCount = 1;

		deviceInfo.pEnabledFeatures = &deviceFeatures;

		deviceInfo.enabledExtensionCount = static_cast<uint32_t>(deviceExtensions.size());
		deviceInfo.ppEnabledExtensionNames = deviceExtensions.data();

		deviceInfo.enabledLayerCount = static_cast<uint32_t>(validationLayers.size());
		deviceInfo.ppEnabledLayerNames = validationLayers.data();

	}

	VK_ASSERT(vkCreateDevice(m_physicalDevice, &deviceInfo, nullptr, &m_logicalDevice));
}

uint32_t Graphics::getQueueFamilyIndex(VkQueueFlagBits flag)
{
// requires:
//		m_physicalDevice

	uint32_t queueFamilyCount;
	vkGetPhysicalDeviceQueueFamilyProperties(m_physicalDevice, &queueFamilyCount, nullptr);

	std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
	vkGetPhysicalDeviceQueueFamilyProperties(m_physicalDevice, &queueFamilyCount, queueFamilies.data());

	for (uint32_t i = 0; i < queueFamilyCount; i++)
		if (queueFamilies[i].queueFlags & flag)
			return i;

	throw std::runtime_error("device doesn't support queue family for flag");
}

uint32_t Graphics::getQueueFamilyIndexPresent()
{
// requires:
//		m_physicalDevice
//		m_surface

	uint32_t queueFamilyCount;
	vkGetPhysicalDeviceQueueFamilyProperties(m_physicalDevice, &queueFamilyCount, nullptr);

	std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
	vkGetPhysicalDeviceQueueFamilyProperties(m_physicalDevice, &queueFamilyCount, queueFamilies.data());

	// look for a queue family with both graphics and presentation first
	uint32_t presentIndex = queueFamilyCount;
	for (uint32_t i = 0; i < queueFamilyCount; i++) {
		VkBool32 supported = false;

		VK_ASSERT(vkGetPhysicalDeviceSurfaceSupportKHR(m_physicalDevice, i, m_surface, &supported));
		
		if (!supported) continue;
		
		supported = queueFamilies[i].queueFlags & VkQueueFlagBits::VK_QUEUE_GRAPHICS_BIT;
		if (!supported) presentIndex = i;
			
		return i;
	}

	if (presentIndex != queueFamilyCount) 
		return presentIndex;
	else
		throw std::runtime_error("no queue family on the gpu supports presentation");
}

void Graphics::createWindow(int width, int height)
{
// requires:
//		m_window
//		m_title
	
	glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
	//glfwWindowHint(GLFW_RESIZABLE, GL_FALSE); // for now -> complicated
	m_window = glfwCreateWindow(800, 600, m_title, nullptr, nullptr);

	glfwSetWindowUserPointer(m_window, this);
	glfwSetFramebufferSizeCallback(m_window, callback::frameBufferSizeCallback);

}

void Graphics::createSurface()
{
// requires:
//		m_instance
//		m_window
//		m_surface
	VkWin32SurfaceCreateInfoKHR surfaceInfo{};
	{
		surfaceInfo.sType = VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR;
		surfaceInfo.hwnd = glfwGetWin32Window(m_window);
		surfaceInfo.hinstance = GetModuleHandle(nullptr);
	}

	VK_ASSERT(glfwCreateWindowSurface(m_instance, m_window, nullptr, &m_surface));
}

void Graphics::createSwapChain()
{
// requires:
//		m_physicalDevice
//		m_surface
//		m_logicalDevice
//		m_swapChain
//		m_swapChainFormat
	VkSurfaceCapabilitiesKHR capabilities{};
	{
		vkGetPhysicalDeviceSurfaceCapabilitiesKHR(m_physicalDevice, m_surface, &capabilities);
	}
	
	// surface format
	VkSurfaceFormatKHR surfaceFormat{};
	{
		uint32_t formatCount;
		VK_ASSERT(vkGetPhysicalDeviceSurfaceFormatsKHR(m_physicalDevice, m_surface, &formatCount, nullptr));
		
		std::vector<VkSurfaceFormatKHR> availableFormats(formatCount);
		VK_ASSERT(vkGetPhysicalDeviceSurfaceFormatsKHR(m_physicalDevice, m_surface, &formatCount, availableFormats.data()));

		surfaceFormat = availableFormats[0];

		m_swapChainFormat = surfaceFormat.format;
	}

	// presentation mode
	{
		uint32_t presentModeCount;
		vkGetPhysicalDeviceSurfacePresentModesKHR(m_physicalDevice, m_surface, &presentModeCount, nullptr);

		std::vector<VkPresentModeKHR> availablePresentModes(presentModeCount);
		VK_ASSERT(vkGetPhysicalDeviceSurfacePresentModesKHR(m_physicalDevice, m_surface, &presentModeCount, availablePresentModes.data()));

		m_swapChainPresentMode = availablePresentModes[0];
	}

	// extent
	{
		m_swapChainExtent = capabilities.currentExtent;
	}

	std::vector<uint32_t> queueFamilyIndices{};
	{
		std::set<uint32_t> uniqueIndices {
			getQueueFamilyIndex(VK_QUEUE_GRAPHICS_BIT),
			getQueueFamilyIndexPresent()
		};
		queueFamilyIndices.resize(uniqueIndices.size());
		std::copy(uniqueIndices.begin(), uniqueIndices.end(), queueFamilyIndices.begin());
	}
	
	uint32_t imageCount;
	{
		imageCount = capabilities.minImageCount + 1;
		if (capabilities.maxImageCount > 0 && imageCount > capabilities.maxImageCount) {
			imageCount = capabilities.maxImageCount;
		}
	}

	VkSwapchainCreateInfoKHR swapChainInfo{};
	{
		swapChainInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
		
		swapChainInfo.surface = m_surface;

		// minimum images required + 1 to avoid waiting on internal operations
		swapChainInfo.minImageCount = capabilities.minImageCount + 1; 
		swapChainInfo.imageFormat = surfaceFormat.format;
		swapChainInfo.imageColorSpace = surfaceFormat.colorSpace;
		swapChainInfo.imageExtent = m_swapChainExtent;
		/*the amount of layers each image consists of. always 1 unless vr*/
		swapChainInfo.imageArrayLayers = 1;
		/*what operations the images are used for. for deferred rendering later may 
		prefer IMAGE_USAGE_TRANSFER_DST_BIT and use a memory operation for rendering*/
		swapChainInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

		
		if (queueFamilyIndices.size() > 1) 
		{
			/*images can be used by multiple queue families*/
			swapChainInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
			swapChainInfo.queueFamilyIndexCount = static_cast<uint32_t>(queueFamilyIndices.size());
			swapChainInfo.pQueueFamilyIndices = queueFamilyIndices.data();
		}
		else
		{
			/*images are owned by queue family unless explicity transferred*/
			swapChainInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
			swapChainInfo.queueFamilyIndexCount = 0;
			swapChainInfo.pQueueFamilyIndices = nullptr;
		}

		swapChainInfo.preTransform = capabilities.currentTransform;
		swapChainInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
		swapChainInfo.presentMode = m_swapChainPresentMode;
		swapChainInfo.clipped = VK_TRUE;

		swapChainInfo.oldSwapchain = VK_NULL_HANDLE;
	}
	
	VK_ASSERT(vkCreateSwapchainKHR(m_logicalDevice, &swapChainInfo, nullptr, &m_swapChain));
}

void Graphics::createImageViews() 
{
// requires:
//		m_physicalDevice
//		m_surface
//		m_logicalDevice
//		m_swapChain
//		m_swapChainImages
//		m_swapChainImageViews

	uint32_t imageCount;
	vkGetSwapchainImagesKHR(m_logicalDevice, m_swapChain, &imageCount, nullptr);
	
	m_swapChainImages.resize(imageCount);
	m_swapChainImageViews.resize(imageCount);
	vkGetSwapchainImagesKHR(m_logicalDevice, m_swapChain, &imageCount, m_swapChainImages.data());

	for (size_t i = 0; i < imageCount; i++)
	{
		VkImageViewCreateInfo viewInfo{};
		{
			viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
			viewInfo.image = m_swapChainImages[i];
			viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;

			// match swapchain format
			viewInfo.format = m_swapChainFormat;

			viewInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
			viewInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
			viewInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
			viewInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
			viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
			viewInfo.subresourceRange.baseMipLevel = 0;
			viewInfo.subresourceRange.levelCount = 1;
			viewInfo.subresourceRange.baseArrayLayer = 0;
			viewInfo.subresourceRange.layerCount = 1;
		}

		VK_ASSERT(vkCreateImageView(m_logicalDevice, &viewInfo, nullptr, &m_swapChainImageViews[i]));
	}
}

void Graphics::createRenderPass()
{
// requires:
//		m_logicalDevice
//		m_swapChainFormat
//		m_renderPass
	VkAttachmentDescription colourAttachment{};
	{
		colourAttachment.format = m_swapChainFormat;
		colourAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
		colourAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
		colourAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
		colourAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
		colourAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
		colourAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		colourAttachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
	}

	VkAttachmentReference attachmentRef{};
	{
		attachmentRef.attachment = 0;
		attachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
	}

	VkSubpassDescription subpass{}; 
	{
		subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
		subpass.colorAttachmentCount = 1;
		subpass.pColorAttachments = &attachmentRef;
	}

	VkSubpassDependency dependency{};
	{
		dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
		dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
		dependency.srcAccessMask = 0;

		dependency.dstSubpass = 0;
		dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
		dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
	}

	VkRenderPassCreateInfo renderInfo{};
	{
		renderInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
		renderInfo.attachmentCount = 1;
		renderInfo.pAttachments = &colourAttachment;
		renderInfo.subpassCount = 1;
		renderInfo.pSubpasses = &subpass;
		renderInfo.dependencyCount = 1;
		renderInfo.pDependencies = &dependency;
	}

	VK_ASSERT(vkCreateRenderPass(m_logicalDevice, &renderInfo, nullptr, &m_renderPass));
}

VkShaderModule Graphics::loadShader(std::string& filepath)
{
// requires:
//		m_logicalDevice
	std::ifstream file(filepath, std::ios::ate | std::ios::binary);

	if (!file.is_open())
		throw std::runtime_error("failed to open shader");

	size_t size = static_cast<size_t>(file.tellg());
	std::vector<char> code(size);

	file.seekg(0);
	file.read(code.data(), size);

	file.close();

	VkShaderModuleCreateInfo moduleInfo{};
	moduleInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
	moduleInfo.codeSize = code.size();
	moduleInfo.pCode = reinterpret_cast<const uint32_t*>(code.data());

	VkShaderModule shader;
	VK_ASSERT(vkCreateShaderModule(m_logicalDevice, &moduleInfo, nullptr, &shader));
	return shader;
}

void Graphics::createGraphicsPipeline()
{
// requires:
//		m_swapChainExtent
//		m_pipelineLayout
//		m_renderPass
//		m_graphicsPipeline
	std::string vertFilepath = "Resources/shader.vert.spv";
	std::string fragFilepath = "Resources/shader.frag.spv";

	VkShaderModule vertModule = loadShader(vertFilepath);
	
	VkPipelineShaderStageCreateInfo vertStageInfo{};
	{
		vertStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
		vertStageInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;

		vertStageInfo.module = vertModule;
		vertStageInfo.pName = "main";

		// can specify compile time constants here
		vertStageInfo.pSpecializationInfo = nullptr;
	}
	
	VkShaderModule fragModule = loadShader(fragFilepath);
	
	VkPipelineShaderStageCreateInfo fragStageInfo{};
	{
		fragStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
		fragStageInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;

		fragStageInfo.module = fragModule;
		fragStageInfo.pName = "main";

		// can specify compile time constants here
		fragStageInfo.pSpecializationInfo = nullptr;
	}

	VkPipelineShaderStageCreateInfo shaderStages[] { vertStageInfo, fragStageInfo };

	VkPipelineVertexInputStateCreateInfo vertexInputInfo{};
	{
		vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
		// details for loading vertex data
		vertexInputInfo.vertexBindingDescriptionCount = 0;
		vertexInputInfo.pVertexBindingDescriptions = nullptr; // optional
		vertexInputInfo.vertexAttributeDescriptionCount = 0;
		vertexInputInfo.pVertexAttributeDescriptions = nullptr; // optional
	}

	VkPipelineInputAssemblyStateCreateInfo inputAssemblyInfo{};
	{
	inputAssemblyInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
	inputAssemblyInfo.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
	inputAssemblyInfo.primitiveRestartEnable = VK_FALSE;
	}

	VkPipelineViewportStateCreateInfo viewportStateInfo{}; 
	{
		VkViewport viewport{};
		{
			viewport.x = 0;
			viewport.y = 0;

			viewport.width = (float)m_swapChainExtent.width;
			viewport.height = (float)m_swapChainExtent.height;

			viewport.minDepth = 0.0f;
			viewport.maxDepth = 1.0f;
		}

		VkRect2D scissor{};
		{
			scissor.offset = { 0, 0 };
			scissor.extent = m_swapChainExtent;
		}

		viewportStateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
		viewportStateInfo.viewportCount = 1;
		viewportStateInfo.pViewports = &viewport;
		viewportStateInfo.scissorCount = 1;
		viewportStateInfo.pScissors = &scissor;
	}

	VkPipelineRasterizationStateCreateInfo rasterizerInfo{};
	{
		rasterizerInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
		// if enabled the fragments that are beyond are clamped to them instead of discarded
		rasterizerInfo.depthClampEnable = VK_FALSE;
		// if enabled then geometry never passes through the rasterizer stage
		rasterizerInfo.rasterizerDiscardEnable = VK_FALSE;

		rasterizerInfo.polygonMode = VK_POLYGON_MODE_FILL;
		
		rasterizerInfo.lineWidth = 1.0f;

		rasterizerInfo.cullMode = VK_CULL_MODE_BACK_BIT;
		rasterizerInfo.frontFace = VK_FRONT_FACE_CLOCKWISE;

		rasterizerInfo.depthBiasConstantFactor = 0.0f;
		rasterizerInfo.depthBiasClamp = 0.0f;
		rasterizerInfo.depthBiasSlopeFactor = 0.0f;
	}

	VkPipelineMultisampleStateCreateInfo multisamplerInfo{};
	{
		multisamplerInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
		multisamplerInfo.sampleShadingEnable = VK_FALSE;
		multisamplerInfo.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
		multisamplerInfo.minSampleShading = 1.0f; // Optional
		multisamplerInfo.pSampleMask = nullptr; // Optional
		multisamplerInfo.alphaToCoverageEnable = VK_FALSE; // Optional
		multisamplerInfo.alphaToOneEnable = VK_FALSE; // Optional
	}

	VkPipelineColorBlendAttachmentState colourAttachmentBlendInfo{};
	{
		colourAttachmentBlendInfo.colorWriteMask = 
			VK_COLOR_COMPONENT_R_BIT | 
			VK_COLOR_COMPONENT_G_BIT | 
			VK_COLOR_COMPONENT_B_BIT |
			VK_COLOR_COMPONENT_A_BIT;
		
		colourAttachmentBlendInfo.blendEnable = VK_FALSE;
	/*	alpha blending
		colourBlendInfo.blendEnable = VK_TRUE;
		colourBlendInfo.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
		colourBlendInfo.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
		colourBlendInfo.colorBlendOp = VK_BLEND_OP_ADD;
		colourBlendInfo.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
		colourBlendInfo.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
		colourBlendInfo.alphaBlendOp = VK_BLEND_OP_ADD;
	*/
	}

	VkPipelineColorBlendStateCreateInfo colourBlendingInfo{};
	{
		colourBlendingInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
		colourBlendingInfo.logicOpEnable = VK_FALSE;
		colourBlendingInfo.logicOp = VK_LOGIC_OP_COPY; // Optional
		colourBlendingInfo.attachmentCount = 1;
		colourBlendingInfo.pAttachments = &colourAttachmentBlendInfo;
		colourBlendingInfo.blendConstants[0] = 0.0f; // Optional
		colourBlendingInfo.blendConstants[1] = 0.0f; // Optional
		colourBlendingInfo.blendConstants[2] = 0.0f; // Optional
		colourBlendingInfo.blendConstants[3] = 0.0f; // Optional
	}

	VkPipelineDynamicStateCreateInfo dynamicStateInfo{};
	{
		std::vector<VkDynamicState> dynamicStates = {
			VK_DYNAMIC_STATE_VIEWPORT,
		};
		dynamicStateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
		dynamicStateInfo.dynamicStateCount = static_cast<uint32_t>(dynamicStates.size());
		dynamicStateInfo.pDynamicStates = dynamicStates.data();
	}

	// setup pipelineLayout for uniforms
	{

	}

	VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
	{ 
		pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
		pipelineLayoutInfo.setLayoutCount = 0;
		pipelineLayoutInfo.pSetLayouts = nullptr;

		pipelineLayoutInfo.pushConstantRangeCount = 0; // Optional
		pipelineLayoutInfo.pPushConstantRanges = nullptr; // Optional
	}
	
	VK_ASSERT(vkCreatePipelineLayout(m_logicalDevice, &pipelineLayoutInfo, nullptr, &m_pipelineLayout));

	VkGraphicsPipelineCreateInfo pipelineInfo{};
	{
		pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
		pipelineInfo.stageCount = 2;
		pipelineInfo.pStages = shaderStages;

		pipelineInfo.pVertexInputState = &vertexInputInfo;
		pipelineInfo.pInputAssemblyState = &inputAssemblyInfo;
		pipelineInfo.pViewportState = &viewportStateInfo;
		pipelineInfo.pRasterizationState = &rasterizerInfo;
		pipelineInfo.pMultisampleState = &multisamplerInfo;
		pipelineInfo.pDepthStencilState = nullptr; // Optional
		pipelineInfo.pColorBlendState = &colourBlendingInfo;
		pipelineInfo.pDynamicState = nullptr; // Optional
		pipelineInfo.layout = m_pipelineLayout;
		pipelineInfo.renderPass = m_renderPass;
		pipelineInfo.subpass = 0;
		pipelineInfo.basePipelineHandle = VK_NULL_HANDLE; // Optional
		pipelineInfo.basePipelineIndex = -1;			  // Optional
	}

	VK_ASSERT(vkCreateGraphicsPipelines(m_logicalDevice, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &m_graphicsPipeline));


	vkDestroyShaderModule(m_logicalDevice, vertModule, nullptr);
	vkDestroyShaderModule(m_logicalDevice, fragModule, nullptr);
}

void Graphics::createFramebuffer()
{
// requires:
//		m_renderpass
//		m_swapChainImageView
//		m_swapChainFramebuffers
//		m_logicalDevice
//
	m_swapChainFramebuffers.resize(m_swapChainImageViews.size());
	for (size_t i = 0; i < m_swapChainImageViews.size(); i++) 
	{
		VkImageView attachments[] = { m_swapChainImageViews[i] };

		VkFramebufferCreateInfo framebufferInfo{};
		framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
		framebufferInfo.renderPass = m_renderPass;
		framebufferInfo.attachmentCount = 1;
		framebufferInfo.pAttachments = attachments;
		framebufferInfo.width = m_swapChainExtent.width;
		framebufferInfo.height = m_swapChainExtent.height;
		framebufferInfo.layers = 1;

		VK_ASSERT(vkCreateFramebuffer(m_logicalDevice, &framebufferInfo, nullptr, &m_swapChainFramebuffers[i]));
	}
}

void Graphics::createCommandPool()
{
// requires:
//		m_logicalDevice
//		m_cmdPool
//		getQueueFamilyIndexPresent()
	VkCommandPoolCreateInfo cmdPoolInfo{};
	{
		cmdPoolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
		// allows command buffer to be reordered individually
		cmdPoolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT; 
		cmdPoolInfo.queueFamilyIndex = getQueueFamilyIndexPresent();
	}

	VK_ASSERT(vkCreateCommandPool(m_logicalDevice, &cmdPoolInfo, nullptr, &m_cmdPool));
}

void Graphics::createCommandBuffer()
{
// requires:
//		m_cmdPool
//		m_logicalDevice
	m_cmdBuffers.resize(MAX_FRAMES_IN_FLIGHT);

	VkCommandBufferAllocateInfo allocInfo{};
	{
		allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
		allocInfo.commandPool = m_cmdPool;
		allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
		allocInfo.commandBufferCount = static_cast<uint32_t>(m_cmdBuffers.size());
		
	}

	VK_ASSERT(vkAllocateCommandBuffers(m_logicalDevice, &allocInfo, m_cmdBuffers.data()));
}

void Graphics::recordCommandBuffer(VkCommandBuffer cmdBuffer, uint32_t imageIndex)
{
// requires:
//		m_renderPass
//		m_swapChainExtent
//		m_graphicsPipeline
//		m_swapChainFramebuffers
	VkCommandBufferBeginInfo beginInfo{};
	{
		beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
		beginInfo.flags = 0; // Optional
		beginInfo.pInheritanceInfo = nullptr; // Optional
	}
	
	VK_ASSERT(vkBeginCommandBuffer(cmdBuffer, &beginInfo));

	VkRenderPassBeginInfo renderPassInfo{};
	{
		VkClearValue clearColor = { {{0.0f, 0.0f, 0.0f, 1.0f}} };

		renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
		renderPassInfo.renderPass = m_renderPass;
		renderPassInfo.framebuffer = m_swapChainFramebuffers[imageIndex];
		renderPassInfo.renderArea.offset = { 0, 0 };
		renderPassInfo.renderArea.extent = m_swapChainExtent;

		renderPassInfo.clearValueCount = 1;
		renderPassInfo.pClearValues = &clearColor;
	}

	vkCmdBeginRenderPass(cmdBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

	vkCmdBindPipeline(cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_graphicsPipeline);
	
	vkCmdDraw(cmdBuffer, 3, 1, 0, 0);

	vkCmdEndRenderPass(cmdBuffer);

	VK_ASSERT(vkEndCommandBuffer(cmdBuffer));

}

void Graphics::createSyncObjects()
{
	m_imageAvailableSemaphore.resize(MAX_FRAMES_IN_FLIGHT);
	m_renderFinishedSemaphore.resize(MAX_FRAMES_IN_FLIGHT);
	m_inFlightFence.resize(MAX_FRAMES_IN_FLIGHT);


	VkSemaphoreCreateInfo semaphoreInfo{};
	{
		semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
	}
	VkFenceCreateInfo fenceInfo{};
	{
		fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
		fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;
	}

	for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
		VK_ASSERT(vkCreateSemaphore(m_logicalDevice, &semaphoreInfo, nullptr, &m_imageAvailableSemaphore[i]));
		VK_ASSERT(vkCreateSemaphore(m_logicalDevice, &semaphoreInfo, nullptr, &m_renderFinishedSemaphore[i]));
		VK_ASSERT(vkCreateFence(m_logicalDevice, &fenceInfo, nullptr, &m_inFlightFence[i]));
	}
}

void Graphics::mainloop()
{
	while (!glfwWindowShouldClose(m_window))
	{
		glfwPollEvents();
		drawFrame();
	}

	vkDeviceWaitIdle(m_logicalDevice);
}

void Graphics::drawFrame() {
	// wait for previous frame
	// acquire image from the swap chain
	// record command buffer wihch draws the scene onto that image
	// submit the recorded command buffer
	// present the swap chain image
	
	VK_ASSERT(vkWaitForFences(m_logicalDevice, 1, &m_inFlightFence[m_currentFrame], VK_TRUE, UINT64_MAX));
	
	uint32_t imageIndex; // acquire next image
	{
		VkResult result = vkAcquireNextImageKHR(m_logicalDevice, m_swapChain, UINT64_MAX, m_imageAvailableSemaphore[m_currentFrame], VK_NULL_HANDLE, &imageIndex);
		if (m_framebufferResized || result == VK_ERROR_OUT_OF_DATE_KHR) {
			recreateSwapChain();
			m_framebufferResized = false;
			return;
		}
		else if (result != VK_SUBOPTIMAL_KHR) {
			VK_ASSERT(result);
		}
	}

	VK_ASSERT(vkResetFences(m_logicalDevice, 1, &m_inFlightFence[m_currentFrame]));

	VK_ASSERT(vkResetCommandBuffer(m_cmdBuffers[m_currentFrame], /*VkCommandBufferResetFlagBits*/0));
	recordCommandBuffer(m_cmdBuffers[m_currentFrame], imageIndex);

	VkPipelineStageFlags waitStages[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };

	VkSemaphore waitSemaphores[] = { m_imageAvailableSemaphore[m_currentFrame] };
	VkSemaphore signalSemaphores[] = { m_renderFinishedSemaphore[m_currentFrame] };

	VkCommandBuffer commandBuffers[] = { m_cmdBuffers[m_currentFrame] };
	
	VkSubmitInfo submitInfo{};
	{
		submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
		submitInfo.waitSemaphoreCount = 1;
		submitInfo.pWaitSemaphores = waitSemaphores;
		submitInfo.pWaitDstStageMask = waitStages;
		submitInfo.commandBufferCount = 1;
		submitInfo.pCommandBuffers = commandBuffers;
		submitInfo.signalSemaphoreCount = 1;
		submitInfo.pSignalSemaphores = signalSemaphores;
	}
	
	VK_ASSERT(vkQueueSubmit(m_graphicsQueue, 1, &submitInfo, m_inFlightFence[m_currentFrame]));

	// present swap chain image to screen
	VkPresentInfoKHR presentInfo{};
	{
		VkSwapchainKHR swapChains[] = { m_swapChain };

		presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
		presentInfo.waitSemaphoreCount = 1;
		presentInfo.pWaitSemaphores = signalSemaphores;
		presentInfo.swapchainCount = 1;
		presentInfo.pSwapchains = swapChains;
		presentInfo.pImageIndices = &imageIndex;
		presentInfo.pResults = nullptr;
	}

	// queue present
	{
		VkResult result = vkQueuePresentKHR(m_presentQueue, &presentInfo);
		if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR) {
			recreateSwapChain();
			return;
		}
		else 
			VK_ASSERT(result);
	}

	// iterate frame
	m_currentFrame = (m_currentFrame + 1) % MAX_FRAMES_IN_FLIGHT;
}

void Graphics::recreateSwapChain()
{
	int width = 0, height = 0;
	glfwGetFramebufferSize(m_window, &width, &height);
	while (width == 0 || height == 0) {
		glfwGetFramebufferSize(m_window, &width, &height);
		glfwWaitEvents();
	}

	vkDeviceWaitIdle(m_logicalDevice);

	cleanupSwapChain();

	createSwapChain();
	createImageViews();
	createRenderPass();
	createGraphicsPipeline();
	createFramebuffer();

}

void Graphics::cleanupSwapChain()
{
	for (size_t i = 0; i < m_swapChainFramebuffers.size(); i++)
		vkDestroyFramebuffer(m_logicalDevice, m_swapChainFramebuffers[i], nullptr);

	vkDestroyPipeline(m_logicalDevice, m_graphicsPipeline, nullptr);
	vkDestroyPipelineLayout(m_logicalDevice, m_pipelineLayout, nullptr);
	vkDestroyRenderPass(m_logicalDevice, m_renderPass, nullptr);

	for (auto& imageView : m_swapChainImageViews)
		vkDestroyImageView(m_logicalDevice, imageView, nullptr);
	vkDestroySwapchainKHR(m_logicalDevice, m_swapChain, nullptr);
}
