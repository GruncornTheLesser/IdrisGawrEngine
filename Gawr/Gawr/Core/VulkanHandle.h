#pragma once

#ifdef GAWR_VULKAN_IMPLEMENTATION
	
#define VKTYPE(TYPE) TYPE
#define VKENUM(ENUM) ENUM

#else

#define VKTYPE(TYPE) void*
#define VKENUM(ENUM) int

#endif

#ifdef _DEBUG
#include <iostream>

#define VK_ASSERT(x) {								\
	VkResult res = x;								\
	if (res != VK_SUCCESS) {						\
		std::cerr << "[Vulkan Error]" << "line : "	\
		<< __LINE__ << " - "						\
		<< string_VkResult(res) << std::endl;		\
		throw std::exception();						\
		}											\
	}
#else
#define VK_ASSERT(x) x;
#endif