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
	VkResult res = x;								\
	if (res != VK_SUCCESS) {						\
		std::cerr << "[Vulkan Error]" << "line : "	\
		<< __LINE__ << " - "						\
		<< string_VkResult(res) << std::endl;		\
		throw std::exception();						\
		}											\
	}
#else
#define VK_ASSERT(x) x;
#endif

#ifdef _DEBUG
#define GLFW_ASSERT(x) {							\
	int res = x;									\
	if (res != GLFW_TRUE)							\
		throw std::exception();						\
	}
#else
#define GLFW_ASSERT(x) x;
#endif


#define DEFAULT_GPU 1

void Application::Callback::glfwError(int, const char* message)
{
	std::cout << "[GLFW Error] - " << message << std::endl;
}

Application::Application(const char* name)
{
	// context
	int res = glfwInit();
	assert(res == GLFW_TRUE);

	createVkInstance();

	getPhysicalDevice();

	createLogicalDevice();



	// window
	createWindow(800, 600, name);

	createRenderPass();

	createSwapChain();

	createImages();


	
	createCommandPool();

	createFrames(); // creates cmd buffer semaphores and fences for each frame. 

	// get queues for render pass
	vkGetDeviceQueue(m_logicalDevice, getQueueFamilyIndex(VK_QUEUE_GRAPHICS_BIT), 0, &m_graphicsQueue);
	vkGetDeviceQueue(m_logicalDevice, getQueueFamilyIndexPresent(), 0, &m_presentQueue);

	createGraphicsPipeline();
}

Application::~Application()
{
	for (uint32_t i = 0; i < m_frames.size(); i++) {
		vkDestroySemaphore(m_logicalDevice, m_frames[i].m_imageAvailable, nullptr);
		vkDestroySemaphore(m_logicalDevice, m_frames[i].m_renderFinished, nullptr);
		vkDestroyFence(m_logicalDevice, m_frames[i].m_inFlight, nullptr);
	}

	vkDestroyCommandPool(m_logicalDevice, m_cmdPool, nullptr);
	
	vkDestroyPipeline(m_logicalDevice, m_graphicsPipeline, nullptr);
	vkDestroyPipelineLayout(m_logicalDevice, m_pipelineLayout, nullptr);


	for (auto& image : m_swapChainImages) {
		vkDestroyFramebuffer(m_logicalDevice, image.m_framebuffer, nullptr);
		vkDestroyImageView(m_logicalDevice, image.m_view, nullptr);
	}
	
	vkDestroyRenderPass(m_logicalDevice, m_renderPass, nullptr);

	vkDestroySwapchainKHR(m_logicalDevice, m_swapChain, nullptr);
	
	vkDestroySurfaceKHR(m_instance, m_surface, nullptr);
	glfwDestroyWindow(m_window);

	vkDestroyDevice(m_logicalDevice, nullptr);
	vkDestroyInstance(m_instance, nullptr);
}

void Application::createVkInstance()
{
#if _DEBUG
// check validation layer support
	uint32_t layerCount;
	VK_ASSERT(vkEnumerateInstanceLayerProperties(&layerCount, nullptr));

	std::vector<VkLayerProperties> layerProperties(layerCount);
	VK_ASSERT(vkEnumerateInstanceLayerProperties(&layerCount, layerProperties.data()));

	for (const char* validationLayerName : validationLayers)
	{
		auto it = std::find_if(layerProperties.begin(), layerProperties.end(), [validationLayerName](VkLayerProperties p) {
			return strcmp(validationLayerName, p.layerName) == 0;
			});

		if (it == layerProperties.end())
			throw std::runtime_error("validation layer requested but not available");
	}
#endif

	uint32_t extensionCount;
	VK_ASSERT(vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, nullptr));

	std::vector<VkExtensionProperties> availableExtensions(extensionCount);
	VK_ASSERT(vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, availableExtensions.data()));

	for (const char* extensionName : instanceExtensions) {
		auto it = std::find_if(availableExtensions.begin(), availableExtensions.end(), [extensionName](VkExtensionProperties p) {
			return strcmp(extensionName, p.extensionName) == 0;
			});

		if (it == availableExtensions.end())
			throw std::runtime_error("vulkan version doesnt support required extensions");
	}

	uint32_t glfwExtensionCount;
	const char** glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);
	
	std::vector<const char*> extensions(glfwExtensionCount + instanceExtensions.size());

	std::set_union(glfwExtensions, glfwExtensions + glfwExtensionCount, instanceExtensions.begin(), instanceExtensions.end(), extensions.begin());

	// get vulkan extensions required for glfw
	VkApplicationInfo appInfo{};
	{
		appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
		appInfo.pApplicationName = "GawrEngine";
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

void Application::getPhysicalDevice()
{
	uint32_t deviceCount = 0;
	VK_ASSERT(vkEnumeratePhysicalDevices(m_instance, &deviceCount, nullptr));

	assert(deviceCount != 0);
	
	std::vector<VkPhysicalDevice> devices(deviceCount);
	VK_ASSERT(vkEnumeratePhysicalDevices(m_instance, &deviceCount, devices.data()));

	// TODO: get device choice from setting.json
	m_physicalDevice = devices[DEFAULT_GPU];
}

void Application::createLogicalDevice()
{
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
	// query device extensions
	{
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

	VkPhysicalDeviceFeatures deviceFeatures{};
	{
		vkGetPhysicalDeviceFeatures(m_physicalDevice, &deviceFeatures);
	}

	uint32_t queueFamilyCount;
	vkGetPhysicalDeviceQueueFamilyProperties(m_physicalDevice, &queueFamilyCount, nullptr);

	std::vector<VkDeviceQueueCreateInfo> queueInfos(queueFamilyCount);
	for (uint32_t i = 0; i < queueFamilyCount; i++) {
		float priority = 1.0;

		queueInfos[i].sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
		queueInfos[i].queueFamilyIndex = i;
		queueInfos[i].queueCount = 1;
		queueInfos[i].pQueuePriorities = &priority;
		queueInfos[i].pNext = nullptr;
	}

	VkDeviceCreateInfo deviceInfo{};
	{
		deviceInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
		deviceInfo.pQueueCreateInfos = queueInfos.data();
		deviceInfo.queueCreateInfoCount = static_cast<uint32_t>(queueInfos.size());

		deviceInfo.pEnabledFeatures = &deviceFeatures;

		deviceInfo.enabledExtensionCount = static_cast<uint32_t>(deviceExtensions.size());
		deviceInfo.ppEnabledExtensionNames = deviceExtensions.data();

		deviceInfo.enabledLayerCount = static_cast<uint32_t>(validationLayers.size());
		deviceInfo.ppEnabledLayerNames = validationLayers.data();

	}

	VK_ASSERT(vkCreateDevice(m_physicalDevice, &deviceInfo, nullptr, &m_logicalDevice));
}

uint32_t Application::getQueueFamilyIndex(VkQueueFlagBits flag)
{
	uint32_t queueFamilyCount;
	vkGetPhysicalDeviceQueueFamilyProperties(m_physicalDevice, &queueFamilyCount, nullptr);

	std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
	vkGetPhysicalDeviceQueueFamilyProperties(m_physicalDevice, &queueFamilyCount, queueFamilies.data());

	for (uint32_t i = 0; i < queueFamilyCount; i++)
		if (queueFamilies[i].queueFlags & flag)
			return i;

	throw std::runtime_error("device doesn't support queue family for flag");
}

uint32_t Application::getQueueFamilyIndexPresent()
{
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

void Application::createWindow(int width, int height, const char* title, bool fullscreen)
{
	glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);

	m_window = glfwCreateWindow(width, height, title, fullscreen ? glfwGetPrimaryMonitor() : nullptr, nullptr);
	assert(m_window != nullptr);

	glfwSetErrorCallback(Callback::glfwError);

	glfwSetWindowUserPointer(m_window, this);

	glfwCreateWindowSurface(m_instance, m_window, nullptr, &m_surface);
}

void Application::createSwapChain()
{
	VkSurfaceCapabilitiesKHR capabilities{};
	{
		vkGetPhysicalDeviceSurfaceCapabilitiesKHR(m_physicalDevice, m_surface, &capabilities);
		m_swapChainExtent = capabilities.currentExtent;
	}
	
	// surface format
	VkSurfaceFormatKHR surfaceFormat{};
	{
		uint32_t formatCount;
		VK_ASSERT(vkGetPhysicalDeviceSurfaceFormatsKHR(m_physicalDevice, m_surface, &formatCount, nullptr));
		
		std::vector<VkSurfaceFormatKHR> availableFormats(formatCount);
		VK_ASSERT(vkGetPhysicalDeviceSurfaceFormatsKHR(m_physicalDevice, m_surface, &formatCount, availableFormats.data()));
		
		surfaceFormat = availableFormats[0];

		m_surfaceFormat = surfaceFormat.format;
	}

	// presentation mode
	{
		uint32_t presentModeCount;
		vkGetPhysicalDeviceSurfacePresentModesKHR(m_physicalDevice, m_surface, &presentModeCount, nullptr);

		std::vector<VkPresentModeKHR> availablePresentModes(presentModeCount);
		VK_ASSERT(vkGetPhysicalDeviceSurfacePresentModesKHR(m_physicalDevice, m_surface, &presentModeCount, availablePresentModes.data()));

		m_swapChainPresentMode = availablePresentModes[0];
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
		swapChainInfo.minImageCount = imageCount;
		swapChainInfo.imageFormat = surfaceFormat.format;
		swapChainInfo.imageColorSpace = surfaceFormat.colorSpace;
		swapChainInfo.imageExtent = m_swapChainExtent;
		swapChainInfo.imageArrayLayers = 1;
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

void Application::createImages() {
	uint32_t imageCount;
	vkGetSwapchainImagesKHR(m_logicalDevice, m_swapChain, &imageCount, nullptr);
	
	std::vector<VkImage> images(imageCount);
	vkGetSwapchainImagesKHR(m_logicalDevice, m_swapChain, &imageCount, images.data());
	
	m_swapChainImages.resize(imageCount);
	for (size_t i = 0; i < imageCount; i++)
	{
		VkImageViewCreateInfo viewInfo{};
		{
			viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
			viewInfo.image = images[i];
			viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;

			// match swapchain format
			viewInfo.format = m_surfaceFormat;

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

		VK_ASSERT(vkCreateImageView(m_logicalDevice, &viewInfo, nullptr, &m_swapChainImages[i].m_view));

		VkFramebufferCreateInfo framebufferInfo{};
		{
			framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
			framebufferInfo.renderPass = m_renderPass;
			framebufferInfo.attachmentCount = 1;
			framebufferInfo.pAttachments = &m_swapChainImages[i].m_view;
			framebufferInfo.width = m_swapChainExtent.width;
			framebufferInfo.height = m_swapChainExtent.height;
			framebufferInfo.layers = 1;
		}

		VK_ASSERT(vkCreateFramebuffer(m_logicalDevice, &framebufferInfo, nullptr, &m_swapChainImages[i].m_framebuffer));
	}
}

VkShaderModule Application::loadShader(std::string& filepath)
{
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

void Application::createGraphicsPipeline()
{
	std::string vertFilepath = "Resources/shader.vert.spv";
	std::string fragFilepath = "Resources/shader.frag.spv";

	VkShaderModule vertModule = loadShader(vertFilepath);
	VkShaderModule fragModule = loadShader(fragFilepath);
	std::vector<VkPipelineShaderStageCreateInfo> shaderStages;
	{
		VkPipelineShaderStageCreateInfo vertStageInfo{};
		{
			vertStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
			vertStageInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;

			vertStageInfo.module = vertModule;
			vertStageInfo.pName = "main";

			// can specify compile time constants here
			vertStageInfo.pSpecializationInfo = nullptr;
		}
	
		VkPipelineShaderStageCreateInfo fragStageInfo{};
		{
			fragStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
			fragStageInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;

			fragStageInfo.module = fragModule;
			fragStageInfo.pName = "main";

			// can specify compile time constants here
			fragStageInfo.pSpecializationInfo = nullptr;
		}

		shaderStages = { vertStageInfo, fragStageInfo };
	}

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

	std::vector<VkDynamicState> dynamicStates = {
			VK_DYNAMIC_STATE_VIEWPORT,
			VK_DYNAMIC_STATE_SCISSOR
	};

	VkPipelineDynamicStateCreateInfo dynamicStateInfo{};
	{
		dynamicStateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
		dynamicStateInfo.dynamicStateCount = static_cast<uint32_t>(dynamicStates.size());
		dynamicStateInfo.pDynamicStates = dynamicStates.data();
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
		pipelineInfo.stageCount = static_cast<uint32_t>(shaderStages.size());
		pipelineInfo.pStages = shaderStages.data();

		pipelineInfo.pVertexInputState = &vertexInputInfo;
		pipelineInfo.pInputAssemblyState = &inputAssemblyInfo;
		pipelineInfo.pViewportState = &viewportStateInfo;
		pipelineInfo.pRasterizationState = &rasterizerInfo;
		pipelineInfo.pMultisampleState = &multisamplerInfo;
		pipelineInfo.pDepthStencilState = nullptr;				// Optional
		pipelineInfo.pColorBlendState = &colourBlendingInfo;
		pipelineInfo.pDynamicState = &dynamicStateInfo;			// Optional
		pipelineInfo.layout = m_pipelineLayout;
		pipelineInfo.renderPass = m_renderPass;
		pipelineInfo.subpass = 0;
		pipelineInfo.basePipelineHandle = VK_NULL_HANDLE;		// Optional
		pipelineInfo.basePipelineIndex = -1;					// Optional
	}

	VK_ASSERT(vkCreateGraphicsPipelines(m_logicalDevice, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &m_graphicsPipeline));

	vkDestroyShaderModule(m_logicalDevice, vertModule, nullptr);
	vkDestroyShaderModule(m_logicalDevice, fragModule, nullptr);
}

void Application::createRenderPass()
{
	// surface format
	VkSurfaceFormatKHR surfaceFormat{};
	{
		uint32_t formatCount;
		VK_ASSERT(vkGetPhysicalDeviceSurfaceFormatsKHR(m_physicalDevice, m_surface, &formatCount, nullptr));

		std::vector<VkSurfaceFormatKHR> availableFormats(formatCount);
		VK_ASSERT(vkGetPhysicalDeviceSurfaceFormatsKHR(m_physicalDevice, m_surface, &formatCount, availableFormats.data()));

		surfaceFormat = availableFormats[0];
	}

	std::vector<VkAttachmentDescription> colourAttachments(1);
	{
		colourAttachments[0].format = surfaceFormat.format;
		colourAttachments[0].samples = VK_SAMPLE_COUNT_1_BIT;
		colourAttachments[0].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
		colourAttachments[0].storeOp = VK_ATTACHMENT_STORE_OP_STORE;
		colourAttachments[0].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
		colourAttachments[0].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
		colourAttachments[0].initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		colourAttachments[0].finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
	}

	std::vector<VkAttachmentReference> attachmentRefs(colourAttachments.size());
	{
		attachmentRefs[0].attachment = 0;
		attachmentRefs[0].layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
	}

	VkSubpassDescription subpass{};
	{
		subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
		subpass.colorAttachmentCount = static_cast<uint32_t>(attachmentRefs.size());
		subpass.pColorAttachments = attachmentRefs.data();
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
		renderInfo.attachmentCount = static_cast<uint32_t>(colourAttachments.size());
		renderInfo.pAttachments = colourAttachments.data();
		renderInfo.subpassCount = 1;
		renderInfo.pSubpasses = &subpass;
		renderInfo.dependencyCount = 1;
		renderInfo.pDependencies = &dependency;
	}

	VK_ASSERT(vkCreateRenderPass(m_logicalDevice, &renderInfo, nullptr, &m_renderPass));
}

void Application::createCommandPool()
{
	VkCommandPoolCreateInfo cmdPoolInfo{};
	{
		cmdPoolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
		// allows command buffer to be reordered individually
		cmdPoolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT; 
		cmdPoolInfo.queueFamilyIndex = getQueueFamilyIndexPresent();
	}

	VK_ASSERT(vkCreateCommandPool(m_logicalDevice, &cmdPoolInfo, nullptr, &m_cmdPool));
}

void Application::createFrames()
{
	VkCommandBufferAllocateInfo cmdBufferInfo{};
	{
		cmdBufferInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
		cmdBufferInfo.commandPool = m_cmdPool;
		cmdBufferInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
		cmdBufferInfo.commandBufferCount = 1;
	}

	VkSemaphoreCreateInfo semaphoreInfo{};
	semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
	VkFenceCreateInfo fenceInfo{};
	fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
	fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

	for (uint32_t i = 0; i < m_frames.size(); i++)
	{
		VK_ASSERT(vkAllocateCommandBuffers(m_logicalDevice, &cmdBufferInfo, &m_frames[i].m_cmdBuffer));
		VK_ASSERT(vkCreateSemaphore(m_logicalDevice, &semaphoreInfo, nullptr, &m_frames[i].m_imageAvailable));
		VK_ASSERT(vkCreateSemaphore(m_logicalDevice, &semaphoreInfo, nullptr, &m_frames[i].m_renderFinished));
		VK_ASSERT(vkCreateFence(    m_logicalDevice, &fenceInfo,	 nullptr, &m_frames[i].m_inFlight));
	}
}

void Application::recordCommandBuffer(VkCommandBuffer cmdBuffer, uint32_t imageIndex)
{	
	/** START RENDER PASS **/
	VkRenderPassBeginInfo renderpassBeginInfo{};
	{
		VkClearValue clearColor = { { {0.0f, 0.0f, 0.0f, 1.0f} } };

		renderpassBeginInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
		renderpassBeginInfo.renderPass = m_renderPass;
		renderpassBeginInfo.framebuffer = m_swapChainImages[imageIndex].m_framebuffer;
		renderpassBeginInfo.renderArea.offset = { 0, 0 };
		renderpassBeginInfo.renderArea.extent = m_swapChainExtent;

		renderpassBeginInfo.clearValueCount = 1;
		renderpassBeginInfo.pClearValues = &clearColor;
	}
	vkCmdBeginRenderPass(cmdBuffer, &renderpassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);

	// bind the pipeline
	vkCmdBindPipeline(cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_graphicsPipeline);

	// bind the triangle buffer data
	{
		// viewport
		VkViewport viewport{ 0, 0, (float)m_swapChainExtent.width, (float)m_swapChainExtent.height, 0.0f, 1.0f };
		vkCmdSetViewport(cmdBuffer, 0, 1, &viewport);

		// scissors
		VkRect2D scissor{ { 0, 0 }, m_swapChainExtent };
		vkCmdSetScissor(cmdBuffer, 0, 1, &scissor);
	}
	
	// draw command
	vkCmdDraw(cmdBuffer, 3, 1, 0, 0);

	// end render pass
	vkCmdEndRenderPass(cmdBuffer);

	VK_ASSERT(vkEndCommandBuffer(cmdBuffer));
}

void Application::run()
{
	int currentFrame = 0;
	while (!glfwWindowShouldClose(m_window))
	{
		// poll window events
		glfwPollEvents();
		
		// wait for previous frame
		VK_ASSERT(vkWaitForFences(m_logicalDevice, 1, &m_frames[currentFrame].m_inFlight, VK_TRUE, UINT64_MAX));

		// acquire next image
		uint32_t currentImage;
		{
			VkResult result = vkAcquireNextImageKHR(m_logicalDevice, m_swapChain, UINT64_MAX, m_frames[currentFrame].m_imageAvailable, VK_NULL_HANDLE, &currentImage);
			if (result == VK_ERROR_OUT_OF_DATE_KHR)
			{
				recreateSwapChain();
				return;
			}
			else if (result == VK_SUBOPTIMAL_KHR) {
				return;
			}
			else VK_ASSERT(result);
		}


		//reset current fence & current cmd buffer
		VK_ASSERT(vkResetFences(m_logicalDevice, 1, &m_frames[currentFrame].m_inFlight));
		VK_ASSERT(vkResetCommandBuffer(m_frames[currentFrame].m_cmdBuffer, 0));

		// begin record cmd buffer
		VkCommandBufferBeginInfo beginInfo{};
		beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;



		VK_ASSERT(vkBeginCommandBuffer(m_frames[currentFrame].m_cmdBuffer, &beginInfo));
		recordCommandBuffer(m_frames[currentFrame].m_cmdBuffer, currentImage);

		VkSemaphore waitSemaphores[] = { m_frames[currentFrame].m_imageAvailable };
		VkPipelineStageFlags waitStages[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
		VkSemaphore signalSemaphores[] = { m_frames[currentFrame].m_renderFinished };
		VkCommandBuffer commandBuffers[] = { m_frames[currentFrame].m_cmdBuffer };

		// submit buffers (only 1 cmd buffer atm)
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

		VK_ASSERT(vkQueueSubmit(m_graphicsQueue, 1, &submitInfo, m_frames[currentFrame].m_inFlight));
		
		// iterate frame
		currentFrame = (++currentFrame == m_frames.size()) ? 0 : currentFrame;

		// present the swap chain image
		VkPresentInfoKHR presentInfo{};
		{
			presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
			presentInfo.waitSemaphoreCount = 1;
			presentInfo.pWaitSemaphores = signalSemaphores;
			presentInfo.swapchainCount = 1;
			presentInfo.pSwapchains = &m_swapChain;
			presentInfo.pImageIndices = &currentImage;
			presentInfo.pResults = nullptr;
		}

		{
			VkResult result = vkQueuePresentKHR(m_presentQueue, &presentInfo);

			if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR) {
				recreateSwapChain();
				if (result == VK_ERROR_OUT_OF_DATE_KHR)
					continue;
			}
			else VK_ASSERT(result);
		}

		VK_ASSERT(vkDeviceWaitIdle(m_logicalDevice));
	}
}

void Application::recreateSwapChain()
{
	// idle while minimized
	int width, height;
	glfwGetFramebufferSize(m_window, &width, &height);
	while (width == 0 || height == 0) {
		glfwGetFramebufferSize(m_window, &width, &height);
		glfwWaitEvents();
	}
	
	vkDeviceWaitIdle(m_logicalDevice);

	for (auto& image : m_swapChainImages) {
		vkDestroyFramebuffer(m_logicalDevice, image.m_framebuffer, nullptr);
		vkDestroyImageView(m_logicalDevice, image.m_view, nullptr);
	}
	vkDestroyRenderPass(m_logicalDevice, m_renderPass, nullptr);

	vkDestroySwapchainKHR(m_logicalDevice, m_swapChain, nullptr);

	createSwapChain();

	createRenderPass();

	createImages();
}