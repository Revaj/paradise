#pragma once

#include <vulkan/vulkan.h>
#include "../../core/asserts.h"


#define VK_CHECK(expr)				\
	{								\
		KASSERT(expr == VK_SUCCESS, "FIRE!");\
	}

typedef struct vulkan_context {
	VkInstance instance;
	VkAllocationCallbacks* allocator;

#if defined(_DEBUG) || 1
	VkDebugUtilsMessengerEXT debug_messenger;
#endif
} vulkan_context;