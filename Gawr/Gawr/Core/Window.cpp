#define GAWR_IMP
#include "Window.h"

#define GLFW_EXPOSE_NATIVE_WIN32
#include <glfw3native.h>

void Gawr::Core::Window::Callback::glfwError(int, const char* msg)
{
	std::cout << "[GLFW Error] - " << msg << std::endl;
}

void Gawr::Core::Window::Callback::glfwResize(GLFWwindow* glfwWindow, int width, int height)
{
	Gawr::Core::Window* wnd = static_cast<Gawr::Core::Window*>(glfwGetWindowUserPointer(glfwWindow));
	wnd->m_resized = true;
}

Gawr::Core::Window::Window(const char* title, uint32_t width, uint32_t height, bool resizing) : Context(title)
{
	createWindow(width, height, title, resizing);

	createSurface();

	createSwapchain();

	createRenderpass();

	createImages();
}

Gawr::Core::Window::~Window()
{
	for (auto& image : m_swapchainImages)
	{
		vkDestroyFramebuffer(Context::logical, image.m_framebuffer, nullptr);
		vkDestroyImageView(Context::logical, image.m_view, nullptr);
	}

	vkDestroyRenderPass(Context::logical, Window::m_renderPass, nullptr);

	vkDestroySwapchainKHR(Context::logical, Window::m_swapchain, nullptr);

	vkDestroySurfaceKHR(Context::instance, Window::m_surface, nullptr);

	glfwDestroyWindow(Window::m_window);
}

bool Gawr::Core::Window::closed()
{
	glfwPollEvents();

	if (m_resized) {
		updateSwapchain();
		m_resized = false;
	}

	return glfwWindowShouldClose(Window::m_window);
}

uint32_t Gawr::Core::Window::get_width() const
{
	int width;
	glfwGetFramebufferSize(m_window, &width, nullptr);
	return width;
}

uint32_t Gawr::Core::Window::get_height() const
{
	int height;
	glfwGetFramebufferSize(m_window, nullptr, &height);
	return height;
}

void Gawr::Core::Window::createWindow(uint32_t width, uint32_t height, const char* title, bool resizing)
{
	glfwSetErrorCallback(Window::Callback::glfwError);

	glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
	glfwWindowHint(GLFW_RESIZABLE, resizing ? GLFW_TRUE : GLFW_FALSE);

	Window::m_window = glfwCreateWindow(width, height, title, nullptr, nullptr);
	if (Window::m_window == nullptr)
		throw std::runtime_error("failed to initialize window");

	glfwSetWindowUserPointer(Window::m_window, this);
}

void Gawr::Core::Window::createSurface()
{
	VkWin32SurfaceCreateInfoKHR surfaceInfo{};
	surfaceInfo.sType = VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR;
	surfaceInfo.hwnd = glfwGetWin32Window(Window::m_window);
	surfaceInfo.hinstance = GetModuleHandle(nullptr);
	
	VK_ASSERT(glfwCreateWindowSurface(Context::instance, Window::m_window, nullptr, &(Window::m_surface)));
}

void Gawr::Core::Window::createSwapchain()
{
	
	VkSurfaceCapabilitiesKHR capabilities{};
	vkGetPhysicalDeviceSurfaceCapabilitiesKHR(Context::physical, Window::m_surface, &capabilities);

	uint32_t imageCount = capabilities.minImageCount + 1;
	if (capabilities.maxImageCount > 0 && capabilities.minImageCount + 1 > capabilities.maxImageCount)
		imageCount = capabilities.maxImageCount;

	
	VkSurfaceFormatKHR surfaceFormat{};
	uint32_t formatCount;
	VK_ASSERT(vkGetPhysicalDeviceSurfaceFormatsKHR(Context::physical, Window::m_surface, &formatCount, nullptr));

	std::vector<VkSurfaceFormatKHR> availableFormats(formatCount);
	VK_ASSERT(vkGetPhysicalDeviceSurfaceFormatsKHR(Context::physical, Window::m_surface, &formatCount, availableFormats.data()));

	surfaceFormat = availableFormats[0];

	m_format = surfaceFormat.format;

	std::vector<uint32_t> queueFamilyIndices{};
	uint32_t queueFamilyCount;
	vkGetPhysicalDeviceQueueFamilyProperties(physical, &queueFamilyCount, nullptr);

	std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
	vkGetPhysicalDeviceQueueFamilyProperties(physical, &queueFamilyCount, queueFamilies.data());

	uint32_t graphicsIndex;
	for (graphicsIndex = 0; graphicsIndex < queueFamilyCount; graphicsIndex++) {
		if ((queueFamilies[graphicsIndex].queueFlags & VK_QUEUE_GRAPHICS_BIT) == VK_QUEUE_GRAPHICS_BIT)
			break;
	}

	if (graphicsIndex == queueFamilyCount)
		throw std::runtime_error("device doesnt support graphics operations.");

	queueFamilyIndices.push_back(graphicsIndex);

	VkBool32 surfaceSupport;
	vkGetPhysicalDeviceSurfaceSupportKHR(Context::physical, graphicsIndex, Window::m_surface, &surfaceSupport);

	if (!surfaceSupport)
	{
		uint32_t presentIndex;
		for (presentIndex = 0; presentIndex < queueFamilyCount; presentIndex++)
		{
			vkGetPhysicalDeviceSurfaceSupportKHR(Context::physical, presentIndex, Window::m_surface, &surfaceSupport);
			if (surfaceSupport)
				break;
		}

		if (graphicsIndex == queueFamilyCount)
			throw std::runtime_error("device doesnt support present operations.");

		queueFamilyIndices.push_back(presentIndex);
	}

	VkSwapchainCreateInfoKHR swapChainInfo{};
	swapChainInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
	swapChainInfo.surface = Window::m_surface;
	swapChainInfo.minImageCount = imageCount;
	swapChainInfo.imageFormat = surfaceFormat.format;
	swapChainInfo.imageColorSpace = surfaceFormat.colorSpace;
	swapChainInfo.imageExtent = capabilities.currentExtent;
	swapChainInfo.imageArrayLayers = 1;
	swapChainInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

	//images can be used by multiple queue families
	swapChainInfo.imageSharingMode = queueFamilyIndices.size() > 1 ? VK_SHARING_MODE_CONCURRENT : VK_SHARING_MODE_EXCLUSIVE;
	swapChainInfo.queueFamilyIndexCount = static_cast<uint32_t>(queueFamilyIndices.size());
	swapChainInfo.pQueueFamilyIndices = queueFamilyIndices.data();

	swapChainInfo.preTransform = capabilities.currentTransform;
	swapChainInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
	swapChainInfo.presentMode = VK_PRESENT_MODE_FIFO_KHR;
	swapChainInfo.clipped = VK_TRUE;

	swapChainInfo.oldSwapchain = VK_NULL_HANDLE;

	VK_ASSERT(vkCreateSwapchainKHR(Context::logical, &swapChainInfo, nullptr, &m_swapchain));
}

void Gawr::Core::Window::createImages()
{
	// get swapchain image count
	uint32_t imageCount;
	vkGetSwapchainImagesKHR(Context::logical, m_swapchain, &imageCount, nullptr);

	// get swapchain image count
	std::vector<VkImage> images(imageCount);
	vkGetSwapchainImagesKHR(Context::logical, m_swapchain, &imageCount, images.data());

	// fill image container
	m_swapchainImages.reserve(imageCount);
	for (size_t i = 0; i < imageCount; i++)
	{
		// create image view
		VkImageViewCreateInfo viewInfo{};
		viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
		viewInfo.image = images[i];
		viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;

		// match swapchain format
		viewInfo.format = m_format;
		viewInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
		viewInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
		viewInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
		viewInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
		viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		viewInfo.subresourceRange.baseMipLevel = 0;
		viewInfo.subresourceRange.levelCount = 1;
		viewInfo.subresourceRange.baseArrayLayer = 0;
		viewInfo.subresourceRange.layerCount = 1;

		VkImageView view;
		VK_ASSERT(vkCreateImageView(Context::logical, &viewInfo, nullptr, &view));

		VkFramebufferCreateInfo framebufferInfo{};
		framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
		framebufferInfo.renderPass = m_renderPass;
		framebufferInfo.attachmentCount = 1;
		framebufferInfo.pAttachments = &view;
		framebufferInfo.width = get_width();
		framebufferInfo.height = get_height();
		framebufferInfo.layers = 1;

		VkFramebuffer framebuffer;
		VK_ASSERT(vkCreateFramebuffer(Context::logical, &framebufferInfo, nullptr, &framebuffer));

		m_swapchainImages.push_back(SwapchainImage{ images[i], view, framebuffer });
	}
}

void Gawr::Core::Window::createRenderpass()
{
	std::vector<VkAttachmentDescription> attachments(1);
	{
		attachments[0].format = m_format;
		attachments[0].samples = VK_SAMPLE_COUNT_1_BIT;
		attachments[0].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
		attachments[0].storeOp = VK_ATTACHMENT_STORE_OP_STORE;
		attachments[0].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
		attachments[0].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
		attachments[0].initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		attachments[0].finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
	}

	std::vector<VkAttachmentReference> colourAttachments(1);
	{
		colourAttachments[0].attachment = 0;
		colourAttachments[0].layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
	}

	VkSubpassDescription subpass{ };
	{
		subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
		
		subpass.inputAttachmentCount = 0;
		subpass.pInputAttachments = nullptr;

		subpass.colorAttachmentCount = static_cast<uint32_t>(colourAttachments.size());
		subpass.pColorAttachments = colourAttachments.data();
		
		subpass.pDepthStencilAttachment = nullptr;
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
		renderInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
		renderInfo.pAttachments = attachments.data();
		renderInfo.subpassCount = 1;
		renderInfo.pSubpasses = &subpass;
		renderInfo.dependencyCount = 1;
		renderInfo.pDependencies = &dependency;
	}

	VK_ASSERT(vkCreateRenderPass(Context::logical, &renderInfo, nullptr, &m_renderPass));
}

void Gawr::Core::Window::updateSwapchain()
{
	while (get_width() == 0 || get_height() == 0)
		glfwWaitEvents();
	
	vkDeviceWaitIdle(logical);

	for (auto& image : m_swapchainImages)
	{
		vkDestroyFramebuffer(Context::logical, image.m_framebuffer, nullptr);
		vkDestroyImageView(Context::logical, image.m_view, nullptr);
	}

	vkDestroyRenderPass(logical, m_renderPass, nullptr);

	vkDestroySwapchainKHR(logical, m_swapchain, nullptr);

	createSwapchain();

	createImages();

	createRenderpass();
}