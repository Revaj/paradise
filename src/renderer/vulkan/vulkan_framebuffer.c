#include "vulkan_framebuffer.h"

#include "../../core/gmemory.h"

void vulkan_framebuffer_create(
	vulkan_context* context,
	vulkan_renderpass* renderpass,
	uint32_t width,
	uint32_t height,
	uint32_t attachment_count,
	VkImageView* attachments,
	vulkan_framebuffer* out_framebuffer) {

	out_framebuffer->attachments = gallocate(sizeof(VkImageView) * attachment_count, MEMORY_TAG_RENDERER);
	for (uint32_t i = 0; i < attachment_count; ++i) {
		out_framebuffer->attachments[i] = attachments[i];
	}
	out_framebuffer->renderpass = renderpass;
	out_framebuffer->attachment_count = attachment_count;

	VkFramebufferCreateInfo framebuffer_create_info = { VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO };
	framebuffer_create_info.renderPass = renderpass->handle;
	framebuffer_create_info.attachmentCount = attachment_count;
	framebuffer_create_info.pAttachments = out_framebuffer->attachments;
	framebuffer_create_info.width = width;
	framebuffer_create_info.height = height;
	framebuffer_create_info.layers = 1;

	VK_CHECK(vkCreateFramebuffer(
		context->device.logical_device,
		&framebuffer_create_info,
		context->allocator,
		&out_framebuffer->handle));
}

void vulkan_framebuffer_destroy(vulkan_context* context, vulkan_framebuffer* framebuffer) {
	vkDestroyFramebuffer(context->device.logical_device, framebuffer->handle, context->allocator);
	if (framebuffer->attachments) {
		gfree(framebuffer->attachments, sizeof(VkImageView) * framebuffer->attachment_count, MEMORY_TAG_RENDERER);
		framebuffer->attachments = 0;
	}

	framebuffer->handle = 0;
	framebuffer->attachment_count = 0;
	framebuffer->renderpass = 0;
}