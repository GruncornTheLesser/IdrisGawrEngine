#pragma once
#include "GawrVK.h"
#include <vector>
namespace Gawr::Core {
	class Context {
	public:
		VK_PTR(VkInstance)			instance;
		VK_PTR(VkPhysicalDevice)	physical;
		VK_PTR(VkDevice)			logical;
		Context(const char* name, int deviceIndex = 1);
			
		~Context();
			
	private:
		VK_PTR(VkInstance)			createInstance(const char* appName);
		VK_PTR(VkPhysicalDevice)	createPhysical(uint32_t device_index);
		VK_PTR(VkDevice)			createLogical();

		static std::vector<const char*> validationLayers;
		static std::vector<const char*> instanceExts;
		static std::vector<const char*> deviceExts;
	};
}