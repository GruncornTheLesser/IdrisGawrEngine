#include "Vulkan.h"
#include "Window.h"
#include <assert.h>

Gawr::Core::Window::Window(const DisplaySettings& settings) {
	glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);

	m_window = glfwCreateWindow(
		settings.resolution[0], settings.resolution[1], 
		settings.appName.c_str(), 
		settings.dispayMode == DisplayMode::Fullscreen ? glfwGetPrimaryMonitor() : nullptr, nullptr);
	
	assert(m_window != nullptr);

	// window event system
	//glfwSetErrorCallback(Callback::glfwError);
	//glfwSetWindowUserPointer(m_wnd, this);


}

Gawr::Core::Window::~Window()
{
	glfwDestroyWindow(m_window);
}

