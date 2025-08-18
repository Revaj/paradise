#include "vulkan_fence.h"

#include "../../core/logger.h"

void vulkan_fence_create(
	vulkan_context* context,
	int8_t create_signaled,
	vulkan_fence* out_fence) {
	out_fence->is_signaled = create_signaled;
	VkFenceCreateInfo fence_create_info = { VK_STRUCTURE_TYPE_FENCE_CREATE_INFO };
	if (out_fence->is_signaled) {
		fence_create_info.flags = VK_FENCE_CREATE_SIGNALED_BIT;
	}

	VK_CHECK(vkCreateFence(
	context->device.logical_device,
		&fence_create_info,
		context->allocator,
		&out_fence->handle));
}

void vulkan_fence_destroy(vulkan_context* context, vulkan_fence* fence) {
	if (fence->handle) {
		vkDestroyFence(
			context->device.logical_device,
			fence->handle,
			context->allocator);
		fence->handle = 0;
	}
	fence->is_signaled = false;
}

int8_t vulkan_fence_wait(vulkan_context* context, vulkan_fence* fence, uint64_t timeout_ns) {
	if (!fence->is_signaled) {
		VkResult result = vkWaitForFences(
			context->device.logical_device,
			1,
			&fence->handle,
			1,
			timeout_ns);
		switch (result) {
		case VK_SUCCESS:
			fence->is_signaled = 1;
			return 1;
		case VK_TIMEOUT:
			KWARN("vk_fence_wait - Timed out");
			break;
		case VK_ERROR_DEVICE_LOST:
			KERROR("vk_fence_wait - VK_ERROR_DEVICE_LOST");
			break;
		case VK_ERROR_OUT_OF_HOST_MEMORY:
			KERROR("vk_fence_wait - VK_ERROR_OUT_OF_HOST_MEMORY.");
			break;
		case VK_ERROR_OUT_OF_DEVICE_MEMORY:
			KERROR("vk_fence_wait - VK_ERROR_OUT_OF_DEVICE_MEMORY.");
			break;
		default:
			KERROR("vk_fence_wait - An unknown error has ocurred.");
			break;
		}
	}
	else {
		return 1;
	}

	return 0;
}

void vulkan_fence_reset(vulkan_context* context, vulkan_fence* fence) {
	if (fence->is_signaled) {
		VK_CHECK(vkResetFences(context->device.logical_device, 1, &fence->handle));
		fence->is_signaled = 0;
	}
}