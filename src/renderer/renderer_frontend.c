#include "renderer_frontend.h"

#include "renderer_backend.h"

#include "../core/logger.h"
#include "../core/gmemory.h"

typedef struct renderer_system_state {
	renderer_backend backend;
} renderer_system_state;

static renderer_system_state* state_ptr;

bool renderer_system_initialize(uint64_t* memory_requirement, void*state,
								const char* application_name) {

	*memory_requirement = sizeof(renderer_system_state);
	if (state == 0) {
		return true;
	}
	state_ptr = state;

	renderer_backend_create(RENDERER_BACKEND_TYPE_VULKAN, &state_ptr->backend);
	state_ptr->backend.frame_number = 0;

	if (!state_ptr->backend.initialize(&state_ptr->backend, application_name)) {
		KFATAL("Renderer backend failed to initialize. Shutting down.");
		return false;
	}

	return true;
}

void renderer_system_shutdown(void* state) {
	if (state_ptr) {
		state_ptr->backend.shutdown(&state_ptr->backend);
	}
	state_ptr = 0;
}

bool renderer_begin_frame(float delta_time) {
	if (!state_ptr) {
		return false;
	}
	return state_ptr->backend.begin_frame(&state_ptr->backend, delta_time);
}

bool renderer_end_frame(float delta_time) {
	if (!state_ptr) {
		return false;
	}
	bool result = state_ptr->backend.end_frame(&state_ptr->backend, delta_time);
	state_ptr->backend.frame_number++;
	return result;
}

void renderer_on_resized(uint16_t width, uint16_t height) {
	if (state_ptr) {
		state_ptr->backend.resized(&state_ptr->backend, width, height);
	}
	else {
		KWARN("renderer backend does not exist to accept resize: %i %i", width, height);
	}
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