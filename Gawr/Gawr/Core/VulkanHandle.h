#pragma once

#ifdef GAWR_VULKAN_IMPLEMENTATION
#define VKALTERNATE(TYPE, EQUIVALENT_TYPE) TYPE
#else
#define VKALTERNATE(TYPE, EQUIVALENT_TYPE) EQUIVALENT_TYPE
#endif

#define VKTYPE(TYPE) VKALTERNATE(TYPE, void*)
#define VKENUM(ENUM) VKALTERNATE(ENUM, int)

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