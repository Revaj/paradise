#pragma once

struct platform_state;
struct vulkan_context;

int8_t platform_create_vulkan_surface(
	struct platform_state* plat_state,
	struct vulkan_context* context);

void platform_get_required_extension_names(const char*** names_darray);