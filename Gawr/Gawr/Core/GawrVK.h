#pragma once

#ifdef GAWR_IMP

#define GLFW_INCLUDE_VULKAN
#define VK_USE_PLATFORM_WIN32_KHR
#include <glfw3.h>

#include <vulkan/vulkan.h>
#include <vulkan/vk_enum_string_helper.h>

#include <iostream>

#ifdef _DEBUG
#define VK_ASSERT(x) { VkResult res = x; if (res != VK_SUCCESS) { std::cerr << "[Vulkan Error]" << "line : " << __LINE__ << " - " << string_VkResult(res) << std::endl;	throw std::exception(); } }
#else
#define VK_ASSERT(x) x;
#endif

#define VK_TYPE(type, other) type
#else
#define VK_TYPE(type, other) other
#endif

#define VK_PTR(type) VK_TYPE(type, void*)
#define VK_ENUM(type) VK_TYPE(type, int)
