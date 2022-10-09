#include "Graphics.h"

int main() {

	Graphics graphics;
	
}




/*

#define GLFW_INCLUDE_VULKAN
#define VK_USE_PLATFORM_WIN32_KHR
#define GLFW_INCLUDE_VULKAN
#include <glfw3.h>

#define GLFW_EXPOSE_NATIVE_WIN32
#include <glfw3native.h>

#include <glm.hpp>

#include <stdexcept>
#include <iostream>
#include <optional>
#include <vector>
#include <set>
#include <algorithm>
#include <fstream>

using namespace glm;

#define DEFAULT_GPU 1


const std::vector<const char*> validationLayers = {
#if defined(_DEBUG)
	"VK_LAYER_KHRONOS_validation"
#endif
};

const std::vector<const char*> deviceRequirements = {
	VK_KHR_SWAPCHAIN_EXTENSION_NAME
};

static std::vector<char> compileShader(const std::string& filename) {
	std::ifstream file(filename, std::ios::ate | std::ios::binary);

	if (!file.is_open()) {
		throw std::runtime_error("failed to open file!");
	}

	size_t fileSize = (size_t)file.tellg();
	std::vector<char> code(fileSize);

	file.seekg(0);
	file.read(code.data(), fileSize);
	file.close();

	VkShaderModuleCreateInfo shaderModuleCreateInfo{};
	shaderModuleCreateInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
	shaderModuleCreateInfo.codeSize = code.size();
	shaderModuleCreateInfo.pCode = reinterpret_cast<const uint32_t*>(code.data());

	VkShaderModule shaderModule;

	VkResult result = vkCreateShaderModule(device, &shaderModuleCreateInfo, nullptr, &shaderModule);

	if (result != VK_SUCCESS)
		throw std::runtime_error("failed to load shader module");
}




int main() 
{
	/*
	glm::ivec2 size(800, 600);
	std::string name("Window");

	// create window
	GLFWwindow* window;
	{
		glfwInit();

		glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
		glfwWindowHint(GLFW_RESIZABLE, GL_FALSE); // for now -> complicated

		window = glfwCreateWindow(size.x, size.y, "window", nullptr, nullptr);
	}

	// create instance
	VkInstance instance;
	{
		
	}

	// verify validation layer support
	{
#if defined(_DEBUG)
		// check validation layer support
		uint32_t layerCount;
		vkEnumerateInstanceLayerProperties(&layerCount, nullptr);

		std::vector<VkLayerProperties> vkLayers(layerCount);
		vkEnumerateInstanceLayerProperties(&layerCount, vkLayers.data());

		for (const char* layerName : validationLayers)
		{
			auto property_match = [layerName](VkLayerProperties p) { return strcmp(layerName, p.layerName) == 0; };
			auto it = std::find_if(vkLayers.begin(), vkLayers.end(), property_match);

			if (it == vkLayers.end())
				throw std::runtime_error("validation layer requested but not available");
		}
#endif
	}

	VkSurfaceKHR surface;
	{
		VkWin32SurfaceCreateInfoKHR surfaceCreateInfo{};
		surfaceCreateInfo.sType = VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR;
		surfaceCreateInfo.hwnd = glfwGetWin32Window(window);
		surfaceCreateInfo.hinstance = GetModuleHandle(nullptr);

		VkResult result = glfwCreateWindowSurface(instance, window, nullptr, &surface);

		if (result != VK_SUCCESS)
			throw std::runtime_error("failed to create window surface");
	}

	// get device choice
	VkPhysicalDevice physicalDevice;
	{
		uint32_t deviceCount = 0;
		vkEnumeratePhysicalDevices(instance, &deviceCount, nullptr);

		if (deviceCount == 0)
			throw std::runtime_error("failed to find GPU with vulkan support");

		std::vector<VkPhysicalDevice> devices(deviceCount);
		vkEnumeratePhysicalDevices(instance, &deviceCount, devices.data());

		// TODO: get device choice from setting.json
		int deviceChoice = 1;
		physicalDevice = devices[deviceChoice];

	}

	// validate physical device for Extensions
	{
		uint32_t extensionCount;
		vkEnumerateDeviceExtensionProperties(physicalDevice, nullptr, &extensionCount, nullptr);

		std::vector<VkExtensionProperties> deviceExtensions(extensionCount);
		vkEnumerateDeviceExtensionProperties(physicalDevice, nullptr, &extensionCount, deviceExtensions.data());

		std::set<std::string> extensionNames;
		for (auto& extension : deviceExtensions)
			extensionNames.insert(extension.extensionName);

		for (const auto& extension : deviceRequirements)
			if (!extensionNames.contains(extension))
				throw std::runtime_error("gpu did not meet system requirements");
	}

	// validate physical device: queue family requirements
	uint32_t graphicsQueueIndex = -1;
	uint32_t presentQueueIndex = -1;
	{
		uint32_t queueFamilyCount = 0;
		vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &queueFamilyCount, nullptr);

		std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
		vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &queueFamilyCount, queueFamilies.data());

		// for each queueFamily
		for (uint32_t i = 0; i < queueFamilyCount; i++) {
			VkBool32 graphicsSupport = queueFamilies[i].queueFlags & VK_QUEUE_GRAPHICS_BIT;
			if (!graphicsSupport) continue;

			graphicsQueueIndex = i;
			break;
		}

		if (graphicsQueueIndex == -1)
			throw std::runtime_error("device selected unsuitable");

		for (uint32_t i = 0; i < queueFamilyCount; i++) {
			VkBool32 presentSupport = false;
			vkGetPhysicalDeviceSurfaceSupportKHR(physicalDevice, i, surface, &presentSupport);
			if (!presentSupport) continue;

			presentQueueIndex = i;
			break;
		}
		
		if (presentQueueIndex == -1)
			throw std::runtime_error("device selected unsuitable");
	}

	// get logical device
	VkDevice device;
	{
		// create queue data
		float queuePriority = 1.0f;

		VkDeviceQueueCreateInfo queueCreateInfo{};
		queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
		queueCreateInfo.queueFamilyIndex = graphicsQueueIndex;
		queueCreateInfo.queueCount = 1;
		queueCreateInfo.pQueuePriorities = &queuePriority;

		VkPhysicalDeviceProperties deviceProperties;
		vkGetPhysicalDeviceProperties(physicalDevice, &deviceProperties);

		VkPhysicalDeviceFeatures deviceFeatures;
		vkGetPhysicalDeviceFeatures(physicalDevice, &deviceFeatures);

		VkDeviceCreateInfo deviceCreateInfo{};
		deviceCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
		deviceCreateInfo.pQueueCreateInfos = &queueCreateInfo;
		deviceCreateInfo.queueCreateInfoCount = 1;
		deviceCreateInfo.pEnabledFeatures = &deviceFeatures;
		deviceCreateInfo.enabledExtensionCount = static_cast<uint32_t>(deviceRequirements.size());
		deviceCreateInfo.ppEnabledExtensionNames = deviceRequirements.data();

		deviceCreateInfo.enabledLayerCount = static_cast<uint32_t>(validationLayers.size());
		deviceCreateInfo.ppEnabledLayerNames = validationLayers.data();

		VkResult result = vkCreateDevice(physicalDevice, &deviceCreateInfo, nullptr, &device);

		if (result != VK_SUCCESS)
			throw std::runtime_error("failed to create logical device");
	}

	// get graphics queue
	VkQueue	graphicsQueue;
	{
		vkGetDeviceQueue(device, graphicsQueueIndex, 0, &graphicsQueue);
	}

	// get device variables
	VkSurfaceCapabilitiesKHR capabilities;
	std::vector<VkSurfaceFormatKHR> surfaceFormats;
	std::vector<VkPresentModeKHR> presentMode;
	{
		// get capabilities
		
		vkGetPhysicalDeviceSurfaceCapabilitiesKHR(physicalDevice, surface, &capabilities);

		// get formats
		uint32_t formatCount;
		vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice, surface, &formatCount, nullptr);

		surfaceFormats.resize(formatCount);
		vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice, surface, &formatCount, surfaceFormats.data());
	
		// get present mode
		uint32_t presentModeCount;
		vkGetPhysicalDeviceSurfacePresentModesKHR(physicalDevice, surface, &presentModeCount, nullptr);

		presentMode.resize(presentModeCount);
		vkGetPhysicalDeviceSurfacePresentModesKHR(physicalDevice, surface, &presentModeCount, presentMode.data());
	}

	// validate physical device for swap chain support
	{
		// get formats
		uint32_t formatCount = 0;
		vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice, surface, &formatCount, nullptr);

		if (formatCount == 0)
			throw std::exception("");

		// get present mode
		uint32_t presentModeCount = 0;
		vkGetPhysicalDeviceSurfacePresentModesKHR(physicalDevice, surface, &presentModeCount, nullptr);

		if (presentModeCount == 0)
			throw std::exception("");
	}

	// create swap chain, swap chain images and swap chain image views
	VkSwapchainKHR swapChain;
	std::vector<VkImage> swapChainImages;
	std::vector<VkImageView> swapChainImageViews;
	{
		VkExtent2D swapExtentChoice = capabilities.currentExtent;
		{
			if (capabilities.currentExtent.width == std::numeric_limits<uint32_t>::max())
			{
				int width, height;
				glfwGetFramebufferSize(window, &width, &height);
				swapExtentChoice = {
					static_cast<uint32_t>(width),
					static_cast<uint32_t>(height)
				};
				swapExtentChoice.width = std::clamp(swapExtentChoice.width, capabilities.minImageExtent.width, capabilities.maxImageExtent.width);
				swapExtentChoice.height = std::clamp(swapExtentChoice.height, capabilities.minImageExtent.height, capabilities.maxImageExtent.height);
			}
		}

		VkSurfaceFormatKHR surfaceFormatChoice = surfaceFormats[0];
		{
			for (const auto& surfaceFormat : surfaceFormats)
			{
				if (surfaceFormat.format == VK_FORMAT_B8G8R8A8_SRGB && surfaceFormat.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
					surfaceFormatChoice = surfaceFormat;
					break;
				}
			}
		}

		VkPresentModeKHR presentModeChoice = VK_PRESENT_MODE_FIFO_KHR;
		{
			for (const auto& presentMode : presentMode)
			{
				if (presentMode == VK_PRESENT_MODE_MAILBOX_KHR) {
					presentModeChoice = presentMode;
					break;
				}
			}
		}

		uint32_t imageCount;
		{
			// if maxImageCount 0, there is no limit
			if (capabilities.maxImageCount == 0 && capabilities.minImageCount >= 0)
				imageCount = capabilities.maxImageCount;
			else
				imageCount = capabilities.minImageCount + 1;
		}

		VkSwapchainCreateInfoKHR swapChainCreateInfo{};
		swapChainCreateInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
		swapChainCreateInfo.surface = surface;

		swapChainCreateInfo.minImageCount = imageCount;
		swapChainCreateInfo.imageFormat = surfaceFormatChoice.format;
		swapChainCreateInfo.imageColorSpace = surfaceFormatChoice.colorSpace;
		swapChainCreateInfo.imageExtent = swapExtentChoice;
		swapChainCreateInfo.imageArrayLayers = 1;
		swapChainCreateInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

		uint32_t queueFamilyIndices[] = { graphicsQueueIndex, presentQueueIndex };

		if (graphicsQueueIndex != presentQueueIndex) {
			swapChainCreateInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
			swapChainCreateInfo.queueFamilyIndexCount = 2;
			swapChainCreateInfo.pQueueFamilyIndices = queueFamilyIndices;
		}
		else
		{
			swapChainCreateInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
		}

		swapChainCreateInfo.preTransform = capabilities.currentTransform;
		swapChainCreateInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
		swapChainCreateInfo.presentMode = presentModeChoice;
		swapChainCreateInfo.clipped = VK_TRUE;
		swapChainCreateInfo.oldSwapchain = VK_NULL_HANDLE;

		VkResult result = vkCreateSwapchainKHR(device, &swapChainCreateInfo, nullptr, &swapChain);

		if (result != VK_SUCCESS)
			throw std::runtime_error("failed to create swap chain");

		vkGetSwapchainImagesKHR(device, swapChain, &imageCount, nullptr);
		swapChainImages.resize(imageCount);
		vkGetSwapchainImagesKHR(device, swapChain, &imageCount, swapChainImages.data());

		swapChainImageViews.resize(swapChainImages.size());

		for (size_t i = 0; i < swapChainImages.size(); i++)
		{
			VkImageViewCreateInfo ImageViewCreateInfo{};
			ImageViewCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
			ImageViewCreateInfo.image = swapChainImages[i];
			ImageViewCreateInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
			ImageViewCreateInfo.format = surfaceFormatChoice.format;
			ImageViewCreateInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
			ImageViewCreateInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
			ImageViewCreateInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
			ImageViewCreateInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
			ImageViewCreateInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
			ImageViewCreateInfo.subresourceRange.baseMipLevel = 0;
			ImageViewCreateInfo.subresourceRange.levelCount = 1;
			ImageViewCreateInfo.subresourceRange.baseArrayLayer = 0;
			ImageViewCreateInfo.subresourceRange.layerCount = 1;

			VkResult result = vkCreateImageView(device, &ImageViewCreateInfo, nullptr, &swapChainImageViews[i]);

			if (result != VK_SUCCESS)
				throw std::runtime_error("failed to create image views");
		}
	}

	// create pipeline
	{
		auto vertShaderCode = compileShader("shaders/vert.spv");
		auto fragShaderCode = compileShader("shaders/frag.spv");


	}

	// main loop
	while (!glfwWindowShouldClose(window))
		glfwPollEvents();

	// clean up
	{
		for (auto imageView : swapChainImageViews)
			vkDestroyImageView(device, imageView, nullptr);

		vkDestroySwapchainKHR(device, swapChain, nullptr);

		vkDestroyDevice(device, nullptr);
		vkDestroySurfaceKHR(instance, surface, nullptr);

		vkDestroyInstance(instance, nullptr);
		glfwDestroyWindow(window);
		glfwTerminate();
	}
	return 0;
}
*/