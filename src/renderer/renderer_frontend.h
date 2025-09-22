#pragma once

#include <stdbool.h>
#include <stdint.h>
#include "renderer_types.inl"


bool renderer_system_initialize(uint64_t* memory_requirement, void* state,
								const char* application_name);
void renderer_system_shutdown(void* state);

void renderer_on_resized(uint16_t width, uint16_t height);

int8_t renderer_draw_frame(render_packet* packet);