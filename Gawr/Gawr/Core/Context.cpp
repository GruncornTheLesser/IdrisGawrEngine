#include "Vulkan.h"
#include "Context.h"

#include <vector>
#include <algorithm>
#include <assert.h>

std::vector<const char*> requiredValidationLayers {
#if _DEBUG
	"VK_LAYER_KHRONOS_validation"
#endif
};

std::vector<const char*> requiredInstanceExtensions {
#if _DEBUG
	"VK_EXT_debug_utils"
#endif
};

std::vector<const char*> requiredDeviceExtensions {
	"VK_KHR_swapchain"
};

Gawr::Core::Graphics::Context::Context(DisplaySettings& settings)
{
#pragma region check instance validation layer support
#if _DEBUG 
	uint32_t instanceLayerCount;
	VK_ASSERT(vkEnumerateInstanceLayerProperties(&instanceLayerCount, nullptr));

	std::vector<VkLayerProperties> instanceLayerProperties(instanceLayerCount);
	VK_ASSERT(vkEnumerateInstanceLayerProperties(&instanceLayerCount, instanceLayerProperties.data()));

	for (const char* validationLayerName : requiredValidationLayers)
	{
		auto it = std::find_if(instanceLayerProperties.begin(), instanceLayerProperties.end(), [validationLayerName](VkLayerProperties p) {
			return strcmp(validationLayerName, p.layerName) == 0;
			});

		if (it == instanceLayerProperties.end())
			throw std::runtime_error("validation layer requested but not available");
	}
#endif
#pragma endregion

#pragma region get glfw required instance extensions
	glfwInit(); // glfw needs to be initiated to get the required extensions

	// get glfw required extensions
	uint32_t glfwExtensionCount;
	const char** glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);

	// remove duplicate extensions
	std::vector<const char*> extensions(glfwExtensionCount + requiredInstanceExtensions.size());
	std::set_union(glfwExtensions, glfwExtensions + glfwExtensionCount, requiredInstanceExtensions.begin(), requiredInstanceExtensions.end(), extensions.begin());
#pragma endregion

#pragma region check instance extension support
	// check extension supported
	uint32_t extensionCount;
	VK_ASSERT(vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, nullptr));

	std::vector<VkExtensionProperties> availableExtensions(extensionCount);
	VK_ASSERT(vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, availableExtensions.data()));

	for (const char* extensionName : extensions) {
		auto it = std::find_if(availableExtensions.begin(), availableExtensions.end(), [extensionName](VkExtensionProperties p) {
			return strcmp(extensionName, p.extensionName) == 0;
			});

		if (it == availableExtensions.end())
			throw std::runtime_error("vulkan version doesnt support required extensions");
	}
#pragma endregion

#pragma region create vulkan instance
	VkApplicationInfo appInfo{};
	{
		appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
		appInfo.pApplicationName = settings.appName.c_str();
		appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
		appInfo.pEngineName = "Gawr";
		appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
		appInfo.apiVersion = VK_API_VERSION_1_0;
	}

	VkInstanceCreateInfo instanceInfo{};
	{
		instanceInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
		instanceInfo.pApplicationInfo = &appInfo;

		instanceInfo.enabledExtensionCount = static_cast<uint32_t>(extensions.size());
		instanceInfo.ppEnabledExtensionNames = extensions.data();

		instanceInfo.enabledLayerCount = static_cast<uint32_t>(requiredValidationLayers.size());
		instanceInfo.ppEnabledLayerNames = requiredValidationLayers.data();
	}

	VK_ASSERT(vkCreateInstance(&instanceInfo, nullptr, &m_instance));
#pragma endregion

#pragma region select physical device

	uint32_t deviceCount = 0;
	VK_ASSERT(vkEnumeratePhysicalDevices(m_instance, &deviceCount, nullptr));

	if (settings.device >= deviceCount) throw std::exception("selected device out of range");

	std::vector<VkPhysicalDevice> devices(deviceCount);
	VK_ASSERT(vkEnumeratePhysicalDevices(m_instance, &deviceCount, devices.data()));


	m_physicalDevice = devices[settings.device];
#pragma endregion

#pragma region check device validation layer support
#if _DEBUG
	{
		uint32_t deviceLayerCount;
		VK_ASSERT(vkEnumerateDeviceLayerProperties(m_physicalDevice, &deviceLayerCount, nullptr));

		std::vector<VkLayerProperties> deviceLayerProperties(deviceLayerCount);
		VK_ASSERT(vkEnumerateDeviceLayerProperties(m_physicalDevice, &deviceLayerCount, deviceLayerProperties.data()));

		for (const char* validationLayerName : requiredValidationLayers)
		{
			auto propertyMatch = [validationLayerName](VkLayerProperties p) { return strcmp(validationLayerName, p.layerName) == 0; };
			auto it = std::find_if(deviceLayerProperties.begin(), deviceLayerProperties.end(), propertyMatch);

			if (it == deviceLayerProperties.end())
				throw std::runtime_error("validation layer requested but not available on gpu");
		}
	}
#endif
#pragma endregion

#pragma region check device extension support
	// query device extensions
	{
		uint32_t deviceExtensionCount;
		VK_ASSERT(vkEnumerateDeviceExtensionProperties(m_physicalDevice, nullptr, &deviceExtensionCount, nullptr));

		std::vector<VkExtensionProperties> deviceExtensions(deviceExtensionCount);
		VK_ASSERT(vkEnumerateDeviceExtensionProperties(m_physicalDevice, nullptr, &deviceExtensionCount, deviceExtensions.data()));

		for (const char* extensionName : requiredDeviceExtensions) {
			auto extensionMatch = [extensionName](VkExtensionProperties p) { return strcmp(extensionName, p.extensionName) == 0; };
			auto it = std::find_if(deviceExtensions.begin(), deviceExtensions.end(), extensionMatch);

			if (it == deviceExtensions.end())
				throw std::runtime_error("gpu doesnt support required extension");
		}
	}
#pragma endregion

#pragma region get queue families
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
#pragma endregion

#pragma region create logical device

	VkDeviceCreateInfo deviceInfo{
		VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,			// sType
		nullptr,										// pNext
		0,												// flags
		static_cast<uint32_t>(queueInfos.size()),		// queue create Info count
		queueInfos.data(),								// queue create Info
		static_cast<uint32_t>(requiredValidationLayers.size()),	// enabled layer count
		requiredValidationLayers.data(),						// enabled layer names
		static_cast<uint32_t>(requiredDeviceExtensions.size()), // enabled extensions
		requiredDeviceExtensions.data()							// enabled extension names
	};

	VK_ASSERT(vkCreateDevice(m_physicalDevice, &deviceInfo, nullptr, &m_device));
#pragma endregion
}

Gawr::Core::Graphics::Context::~Context()
{
	vkDestroyDevice(m_device, nullptr);
	vkDestroyInstance(m_instance, nullptr);
	glfwTerminate();
}
