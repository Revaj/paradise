#pragma once

#include "vulkan_types.inl"

int8_t vulkan_device_create(vulkan_context* context);

void vulkan_device_destroy(vulkan_context* vulkan_context);

void vulkan_device_query_swapchain_support(
	VkPhysicalDevice physical_device,
	VkSurfaceKHR surface,
	vulkan_swapchain_support_info* out_support_info);