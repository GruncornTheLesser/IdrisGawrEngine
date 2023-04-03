#pragma once
#include "VulkanHandle.h"
#include "Settings.h"

// ToDo:
//	 specify which queues to create and how many -> currently one of each

namespace Gawr::Core {
	struct Context {
	public:
		Context(const DisplaySettings& settings);
		~Context();

		VKTYPE(VkInstance)			m_instance;
		VKTYPE(VkPhysicalDevice)	m_physicalDevice;
		VKTYPE(VkDevice)			m_device;

		int presentFamily;
		int graphicFamily;
		int computeFamily;
	};

}
