#pragma once
#include "VulkanHandle.h"
#include "Context.h"
#include "Window.h"

namespace Gawr::Core {

	struct Swapchain {
		VKTYPE(VkRenderPass) m_renderpass;

		VKTYPE(VkSurfaceKHR) m_surface;
		VKALTERNATE(VkExtent2D, uint64_t) m_extent;
		VKENUM(VkFormat) m_format;

		VKTYPE(VkSwapchainKHR) m_swapchain;
		VKTYPE(VkImage)* m_images;
		VKTYPE(VkImageView)* m_views;
		VKTYPE(VkFramebuffer)* m_framebuffers;

		// create offscreen swapchain
		void create(const Context& context, VKALTERNATE(VkExtent2D, glm::uvec2)extent, VKENUM(VkFormat) format, VKENUM(VkColorSpaceKHR) colorspace);
		// create 
		void create(const Context&, const Window&);
		void destroy(const Context&);

	private:
		void create(const Context&, VKTYPE(VkSurfaceKHR));
	};
}