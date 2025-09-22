#pragma once

#include <stdbool.h>
#include <stdint.h>

typedef enum renderer_backend_type{
	RENDERER_BACKEND_TYPE_VULKAN,
	RENDERER_BACKEND_TYPE_OPENGL,
	RENDERER_BACKEND_TYPE_DIRECTX
} renderer_backend_type;

typedef struct renderer_backend {
	struct platform_state* plat_state;
	uint64_t frame_number;

	bool(*initialize)(struct renderer_backend* backend, const char* application_name);
	void (*shutdown)(struct renderer_backend* backend);
	void (*resized)(struct renderer_backend* backend, uint16_t width, uint16_t height);

	int8_t(*begin_frame)(struct renderer_backend* backend, float delta_time);
	int8_t(*end_frame)(struct renderer_backend* backend, float delta_time);
} renderer_backend;

typedef struct render_packet {
	float delta_time;
} render_packet;