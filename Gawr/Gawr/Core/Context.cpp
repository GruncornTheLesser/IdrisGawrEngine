#define GAWR_IMP
#include "Context.h"
#include <assert.h>
#include <algorithm>


std::vector<const char*> Gawr::Core::Context::instanceExts = {
#if defined(_DEBUG)
	VK_EXT_DEBUG_UTILS_EXTENSION_NAME
#endif
};

std::vector<const char*> Gawr::Core::Context::deviceExts = {
	VK_KHR_SWAPCHAIN_EXTENSION_NAME
};

std::vector<const char*> Gawr::Core::Context::validationLayers = {
#if defined(_DEBUG)
	"VK_LAYER_KHRONOS_validation"
#endif
	
};

Gawr::Core::Context::Context(const char* appName, int deviceIndex) : 
	instance(createInstance(appName)),
	physical(createPhysical(deviceIndex)),
	logical(createLogical())
{ }

Gawr::Core::Context::~Context()
{
	vkDestroyDevice(logical, nullptr);
	vkDestroyInstance(instance, nullptr);
}

VkInstance Gawr::Core::Context::createInstance(const char* appName)
{
	int res = glfwInit();
	assert(res == GLFW_TRUE);

	// check validation layer support
#if _DEBUG
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

	// check extensions
	{
		uint32_t extensionCount;
		VK_ASSERT(vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, nullptr));

		std::vector<VkExtensionProperties> availableExtensions(extensionCount);
		VK_ASSERT(vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, availableExtensions.data()));

		for (const char* extensionName : instanceExts) {
			auto it = std::find_if(availableExtensions.begin(), availableExtensions.end(), [extensionName](VkExtensionProperties p) {
				return strcmp(extensionName, p.extensionName) == 0;
				});

			if (it == availableExtensions.end())
				throw std::runtime_error("vulkan version doesnt support required extensions");
		}
	}

	// get required extensions
	uint32_t glfwExtensionCount;
	const char** glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);
	std::vector<const char*> uniqueExtensions(glfwExtensionCount + instanceExts.size());
	std::set_union(glfwExtensions, glfwExtensions + glfwExtensionCount, instanceExts.begin(), instanceExts.end(), uniqueExtensions.begin());

	// get vulkan extensions required for glfw
	VkApplicationInfo appInfo{};
	{
		appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
		appInfo.pApplicationName = appName;
		appInfo.pEngineName = "GawrEngine";
		appInfo.apiVersion = VK_API_VERSION_1_0;
	}

	// info to create an instance
	VkInstanceCreateInfo instanceInfo{};
	{
		instanceInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
		instanceInfo.pApplicationInfo = &appInfo;

		instanceInfo.enabledExtensionCount = static_cast<uint32_t>(uniqueExtensions.size());
		instanceInfo.ppEnabledExtensionNames = uniqueExtensions.data();

		instanceInfo.enabledLayerCount = static_cast<uint32_t>(validationLayers.size());
		instanceInfo.ppEnabledLayerNames = validationLayers.data();
	}

	VkInstance inst;
	VK_ASSERT(vkCreateInstance(&instanceInfo, nullptr, &inst));
	return inst;
}

VkPhysicalDevice Gawr::Core::Context::createPhysical(uint32_t device_index)
{
	uint32_t deviceCount = 0;
	VK_ASSERT(vkEnumeratePhysicalDevices(instance, &deviceCount, nullptr));

	assert(deviceCount != 0);

	std::vector<VkPhysicalDevice> devices(deviceCount);
	VK_ASSERT(vkEnumeratePhysicalDevices(instance, &deviceCount, devices.data()));

	// check validation layer support
#if defined(_DEBUG)
		uint32_t layerCount;
		VK_ASSERT(vkEnumerateDeviceLayerProperties(devices[device_index], &layerCount, nullptr));

		std::vector<VkLayerProperties> layerProperties(layerCount);
		VK_ASSERT(vkEnumerateDeviceLayerProperties(devices[device_index], &layerCount, layerProperties.data()));

		for (const char* validationLayerName : validationLayers)
		{
			auto propertyMatch = [validationLayerName](VkLayerProperties p) { return strcmp(validationLayerName, p.layerName) == 0; };
			auto it = std::find_if(layerProperties.begin(), layerProperties.end(), propertyMatch);

			if (it == layerProperties.end())
				throw std::runtime_error("validation layer requested but not available on gpu");
		}
#endif
	// check extension support
	uint32_t extensionCount;
	VK_ASSERT(vkEnumerateDeviceExtensionProperties(devices[device_index], nullptr, &extensionCount, nullptr));

	std::vector<VkExtensionProperties> availableExtensions(extensionCount);
	VK_ASSERT(vkEnumerateDeviceExtensionProperties(devices[device_index], nullptr, &extensionCount, availableExtensions.data()));

	for (const char* extensionName : deviceExts) {
		auto it = std::find_if(availableExtensions.begin(), availableExtensions.end(), [extensionName](VkExtensionProperties p) {
			return strcmp(extensionName, p.extensionName) == 0;
			});

		if (it == availableExtensions.end()) {
			std::cerr << "gpu doesnt support required extension" << std::endl;
			throw std::exception();
		}
	}

	return devices[device_index];
}

VkDevice Gawr::Core::Context::createLogical()
{
	VkPhysicalDeviceFeatures deviceFeatures{};
	vkGetPhysicalDeviceFeatures(physical, &deviceFeatures);
	
	uint32_t queueFamilyCount;
	vkGetPhysicalDeviceQueueFamilyProperties(physical, &queueFamilyCount, nullptr);

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
		deviceInfo.queueCreateInfoCount = static_cast<uint32_t>(queueInfos.size());
		deviceInfo.pQueueCreateInfos = queueInfos.data();

		deviceInfo.pEnabledFeatures = &deviceFeatures;

		deviceInfo.enabledExtensionCount = static_cast<uint32_t>(deviceExts.size());
		deviceInfo.ppEnabledExtensionNames = deviceExts.data();

		deviceInfo.enabledLayerCount = static_cast<uint32_t>(validationLayers.size());
		deviceInfo.ppEnabledLayerNames = validationLayers.data();

	}

	VkDevice device;
	VK_ASSERT(vkCreateDevice(physical, &deviceInfo, nullptr, &device));
	return device;
}
