#pragma once
#include "VulkanHandle.h"
#include "Settings.h"

// ToDo:
//	 specify which queues to create and how many -> currently one of each


namespace Gawr::Core::Graphics {
	class Context {
	public:
		Context(DisplaySettings& settings);
		~Context();

	private:
		VKTYPE(VkInstance)			m_instance;
		VKTYPE(VkPhysicalDevice)	m_physicalDevice;
		VKTYPE(VkDevice)			m_device;
	};

}
