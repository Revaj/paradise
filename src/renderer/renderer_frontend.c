#include "renderer_frontend.h"

#include "renderer_backend.h"

#include "../core/logger.h"
#include "../core/gmemory.h"

static renderer_backend* backend = 0;

int8_t renderer_initialize(const char* application_name, struct platform_state* plat_state) {
	backend = gallocate(sizeof(renderer_backend), MEMORY_TAG_RENDERER);

	renderer_backend_create(RENDERER_BACKEND_TYPE_VULKAN, plat_state, backend);
	backend->frame_number = 0;

	if (!backend->initialize(backend, application_name, plat_state)) {
		KFATAL("Renderer backend failed to initialize. Shutting down.");
		return 0;
	}

	return 1;
}

void renderer_shutdown() {
	backend->shutdown(backend);
	gfree(backend, sizeof(renderer_backend), MEMORY_TAG_RENDERER);
}

int8_t renderer_begin_frame(float delta_time) {
	return backend->begin_frame(backend, delta_time);
}

int8_t renderer_end_frame(float delta_time) {
	int8_t result = backend->end_frame(backend, delta_time);
	backend->frame_number++;
	return result;
}

int8_t renderer_draw_frame(render_packet* packet) {
	if (renderer_begin_frame(packet->delta_time)) {
		int8_t result = renderer_end_frame(packet->delta_time);

		if (!result) {
			KERROR("renderer_end_frame failed.Application shutting down...");
			return 0;
		}
	}

	return 1;
}