#pragma once
#include "VulkanHandle.h"
#include "Settings.h"
#include "Context.h"
#include <glm/glm.hpp>
#include <vector>

// ToDo:
//	 get vulkan surface
//	 


namespace Gawr::Core {
	struct Window {
		Window(const DisplaySettings& settings);
		~Window();
		VKTYPE(GLFWwindow*)		m_window;
	};
}


