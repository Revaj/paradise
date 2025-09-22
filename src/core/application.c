#include "application.h"
#include "../game_types.h"
#include "logger.h"
#include "../platform/platform.h"
#include "../core/gmemory.h"
#include "../core/event.h"
#include "clock.h"
#include "input.h"

#include "../memory/linear_allocator.h"

#include "../renderer/renderer_frontend.h"

typedef struct application_state {
	game* game_inst;
	uint8_t is_running;
	uint8_t is_suspended;
	int16_t width;
	int16_t height;
	clock clock;
	double last_time;

	linear_allocator systems_allocator;
	uint64_t event_system_memory_requirement;
	void* event_system_state;

	uint64_t memory_system_memory_requirement;
	void* memory_system_state;

	uint64_t logging_system_memory_requirement;
	void* logging_system_state;

	uint64_t input_system_memory_requirement;
	void* input_system_state;

	uint64_t platform_system_memory_requirement;
	void* platform_system_state;

	uint64_t renderer_system_memory_requirement;
	void* renderer_system_state;


} application_state;

static uint8_t initialized = 0;
static application_state* app_state;

uint8_t application_on_event(uint16_t code, void* sender, void* listener_inst, event_context context);
uint8_t application_on_key(uint16_t code, void* sender, void* listener_inst, event_context context);
uint8_t application_on_resized(uint16_t code, void* sender, void* listener_inst, event_context context);

uint8_t application_create(game* game_inst) {
	if(game_inst->application_state) {
		KERROR("application_create called more than once");
		return 0;
	}

	game_inst->application_state = gallocate(sizeof(game_inst), MEMORY_TAG_APPLICATION);
	app_state = game_inst->application_state;
	app_state->game_inst = game_inst;
	app_state->is_running = true;
	app_state->is_suspended = false;

	uint64_t systems_allocator_total_size = 64 * 1024 * 1024; // 64 mb
	linear_allocator_create(systems_allocator_total_size, 0, &app_state->systems_allocator);

	// Initialize systems

	KERROR("hola eventos");
	event_system_initialize(&app_state->event_system_memory_requirement, 0);
	app_state->event_system_state = linear_allocator_allocate(&app_state->systems_allocator, app_state->event_system_memory_requirement);
	event_system_initialize(&app_state->event_system_memory_requirement, app_state->event_system_state);

	KERROR("hola memoru");
	memory_system_initialize(&app_state->memory_system_memory_requirement, 0);
	app_state->memory_system_state = linear_allocator_allocate(&app_state->systems_allocator, app_state->memory_system_memory_requirement);
	memory_system_initialize(&app_state->memory_system_memory_requirement, app_state->memory_system_state);
	
	KERROR("holalogging");
	initialize_logging(&app_state->logging_system_memory_requirement, 0);
	app_state->logging_system_state = linear_allocator_allocate(&app_state->systems_allocator, app_state->logging_system_memory_requirement);
	if (!initialize_logging(&app_state->logging_system_memory_requirement, app_state->logging_system_state)) {
		KERROR("Failed to initialize logging system; shutting down.");
		return false;
	}

	KERROR("holainput");
	input_system_initialize(&app_state->input_system_memory_requirement, 0);
	app_state->input_system_state = linear_allocator_allocate(&app_state->systems_allocator, app_state->input_system_memory_requirement);
	input_system_initialize(&app_state->input_system_memory_requirement, app_state->input_system_state);
	
	event_register(EVENT_CODE_APPLICATION_QUIT, 0, application_on_event);
	event_register(EVENT_CODE_KEY_PRESSED, 0, application_on_key);
	event_register(EVENT_CODE_KEY_RELEASED, 0, application_on_key);
	event_register(EVENT_CODE_RESIZED, 0, application_on_resized);

	KERROR("Hola platf");
	platform_system_startup(&app_state->platform_system_memory_requirement, 0, 0, 0, 0, 0, 0);
	app_state->platform_system_state = linear_allocator_allocate(&app_state->systems_allocator, app_state->platform_system_memory_requirement);
	if (!platform_system_startup(
		&app_state->platform_system_memory_requirement,
		app_state->platform_system_state, game_inst->app_config.name,
		game_inst->app_config.start_pos_x,
		game_inst->app_config.start_pos_y,
		game_inst->app_config.start_width,
		game_inst->app_config.start_height)) {
		return -1;
	}

	KERROR("hola render");
	renderer_system_initialize(&app_state->renderer_system_memory_requirement, 0, 0);
	app_state->renderer_system_state = linear_allocator_allocate(&app_state->systems_allocator, app_state->renderer_system_memory_requirement);
	if (!renderer_system_initialize(&app_state->renderer_system_memory_requirement, app_state->renderer_system_state, game_inst->app_config.name)) {
		KFATAL("Failed to initialize renderer. Aborting application.");
		return 0;
	}

	if (!app_state->game_inst->initialize(app_state->game_inst)) {
		KFATAL("Game failed to initialize.");
		return -1;
	}

	app_state->game_inst->on_resize(app_state->game_inst, app_state->width, app_state->height);
	initialized = true;

	return 1;

}

uint8_t application_run() {
	clock_start(&app_state->clock);
	clock_update(&app_state->clock);
	app_state->last_time = app_state->clock.elapsed;
	double running_time = 0;
	uint8_t frame_count = 0;
	double target_frame_seconds = 1.0f / 60;
	
	KINFO(get_memory_usage_str());
	while (app_state->is_running) {
		if (!platform_pump_messages()) {
			app_state->is_running = false;
		}

		if (!app_state->is_suspended) {

			clock_update(&app_state->clock);
			double current_time = app_state->clock.elapsed;
			double delta = (current_time - app_state->last_time);
			double frame_start_time = platform_get_absolute_time();

			if (!app_state->game_inst->update(app_state->game_inst, (float)delta)) {
				KFATAL("Game update failed, shutting down.");
				app_state->is_running = 0;
				break;
			}

			if (!app_state->game_inst->render(app_state->game_inst, (float)delta)) {
				KFATAL("Game render failed, shutting down.");
				app_state->is_running = 0;
				break;
			}

			render_packet packet;
			packet.delta_time = (float)delta;
			renderer_draw_frame(&packet);
			double frame_end_time = platform_get_absolute_time();
			double frame_elapsed_time = frame_end_time - frame_start_time;
			running_time += frame_elapsed_time;
			double remaining_seconds = target_frame_seconds - frame_elapsed_time;

			if (remaining_seconds > 0) {
				uint64_t remaining_ms = (remaining_seconds * 1000);
				int8_t limit_frames = 0;
				if (remaining_ms > 0 && limit_frames) {
					platform_sleep(remaining_ms - 1);
				}
				frame_count++;
			}

			input_update(delta);

			app_state->last_time = current_time;
		}
	}

	app_state->is_running = false;

	event_unregister(EVENT_CODE_APPLICATION_QUIT, 0, application_on_event);
	event_unregister(EVENT_CODE_KEY_PRESSED, 0, application_on_key);
	event_unregister(EVENT_CODE_KEY_RELEASED, 0, application_on_key);

	input_system_shutdown(app_state->input_system_state);
	renderer_system_shutdown(app_state->renderer_system_state);
	platform_system_shutdown(app_state->platform_system_state);
	memory_system_shutdown(app_state->memory_system_state);
	event_system_shutdown(app_state->event_system_state);

	return 1;
}

void application_get_framebuffer_size(uint32_t* width, uint32_t* height) {
	*width = app_state->width;
	*height = app_state->height;
}

uint8_t application_on_event(uint16_t code, void* sender, void* listener_inst, event_context context) {
	switch (code) {
		case EVENT_CODE_APPLICATION_QUIT: {
			KINFO("EVENT_CODE_APPLICATION_QUIT received, shutting down.\n");
			app_state->is_running = false;
			return 1;
		}
	}
	return 0;
}

uint8_t application_on_key(uint16_t code, void* sender, void* listener_inst, event_context context) {
	if (code == EVENT_CODE_KEY_PRESSED) {
		uint16_t key_code = context.data.u16[0];
		if (key_code == KEY_ESCAPE) {
			event_context data = {0};
			event_fire(EVENT_CODE_APPLICATION_QUIT, 0, data);

			return 1;
		} else if (key_code == KEY_A) {
			KDEBUG("Explicit - A key pressed!");
		}
		else {
			KDEBUG("'%c' key pressed in window.", key_code);
		}
	}
	else if (code == EVENT_CODE_KEY_RELEASED) {
		uint16_t key_code = context.data.u16[0];
		if (key_code == KEY_B) {
			KDEBUG("Explicit - B key released!");
		} else {
			KDEBUG("'%c' key released in window.", key_code);

		}
	}

	return 0;
}

uint8_t application_on_resized(uint16_t code, void* sender, void* listener_inst, event_context context) {
	if (code == EVENT_CODE_RESIZED) {
		uint16_t width = context.data.u16[0];
		uint16_t height = context.data.u16[1];

		if (width != app_state->width || height != app_state->height) {
			app_state->width = width;
			app_state->height = height;

			KDEBUG("Window resize: %i %i", width, height);

			if (width == 0 || height == 0) {
				KINFO("Window minimized, suspending application.");
				app_state->is_suspended = true;
				return true;
			}
			else {
				if (app_state->is_suspended) {
					KINFO("Window restored, resuming application.");
					app_state->is_suspended = false;
				}
				app_state->game_inst->on_resize(app_state->game_inst, width, height);
				renderer_on_resized(width, height);
			}
		}
	}

	return false;
}