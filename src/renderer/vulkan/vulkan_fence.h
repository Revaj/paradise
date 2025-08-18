#pragma once

#include "vulkan_types.inl"

void vulkan_fence_create(
	vulkan_context* context,
	int8_t create_signaled,
	vulkan_fence* out_fence);

void vulkan_fence_destroy(vulkan_context* context, vulkan_fence* fence);

int8_t vulkan_fence_wait(vulkan_context* context, vulkan_fence* fence, uint64_t timeout_ns);

void vulkan_fence_reset(vulkan_context* context, vulkan_fence* fence);