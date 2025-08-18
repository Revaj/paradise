#include "application.h"
#include "../game_types.h"
#include "logger.h"
#include "../platform/platform.h"
#include "../core/gmemory.h"
#include "../core/event.h"
#include "clock.h"
#include "input.h"

#include "../renderer/renderer_frontend.h"

typedef struct application_state {
	game* game_inst;
	uint8_t is_running;
	uint8_t is_suspended;
	platform_state platform;
	int16_t width;
	int16_t height;
	clock clock;
	double last_time;
} application_state;

static uint8_t initialized = 0;
static application_state app_state;

uint8_t application_on_event(uint16_t code, void* sender, void* listener_inst, event_context context);
uint8_t application_on_key(uint16_t code, void* sender, void* listener_inst, event_context context);
uint8_t application_create(game* game_inst) {
	if (initialized) {
		KERROR("application_create called more than once");
		return 0;
	}

	app_state.game_inst = game_inst;
	initialize_logging();
	input_initialize();

	KFATAL("THIS IS A TEST %f", 500);
	KERROR("A test message: %f", 3.14f);
	KWARN("A test message: %f", 3.14f);
	KINFO("A test message: %f", 3.14f);
	KDEBUG("A test message: %f", 3.14f);
	KTRACE("A test message: %f", 3.14f);

	app_state.is_running = true;
	app_state.is_suspended = false;

	if (!event_initialize()) {
		KERROR("Event system failed initialization. Application cannot continue.");
		return 0;
	}

	event_register(EVENT_CODE_APPLICATION_QUIT, 0, application_on_event);
	event_register(EVENT_CODE_KEY_PRESSED, 0, application_on_key);
	event_register(EVENT_CODE_KEY_RELEASED, 0, application_on_key);

	if (!platform_startup(
		&app_state.platform,
		game_inst->app_config.name,
		game_inst->app_config.start_pos_x,
		game_inst->app_config.start_pos_y,
		game_inst->app_config.start_width,
		game_inst->app_config.start_height)) {
		return -1;
	}

	if (!renderer_initialize(game_inst->app_config.name, &app_state.platform)) {
		KFATAL("Failed to initialize renderer. Aborting application.");
		return 0;
	}

	if (!app_state.game_inst->initialize(app_state.game_inst)) {
		KFATAL("Game failed to initialize.");
		return -1;
	}

	app_state.game_inst->on_resize(app_state.game_inst, app_state.width, app_state.height);
	initialized = true;

	return 1;

}

uint8_t application_run() {
	clock_start(&app_state.clock);
	clock_update(&app_state.clock);
	app_state.last_time = app_state.clock.elapsed;
	double running_time = 0;
	uint8_t frame_count = 0;
	double target_frame_seconds = 1.0f / 60;
	
	KINFO(get_memory_usage_str());
	while (app_state.is_running) {
		if (!platform_pump_messages(&app_state.platform)) {
			app_state.is_running = false;
		}

		if (!app_state.is_suspended) {

			clock_update(&app_state.clock);
			double current_time = app_state.clock.elapsed;
			double delta = (current_time - app_state.last_time);
			double frame_start_time = platform_get_absolute_time();

			if (!app_state.game_inst->update(app_state.game_inst, (float)delta)) {
				KFATAL("Game update failed, shutting down.");
				app_state.is_running = 0;
				break;
			}

			if (!app_state.game_inst->render(app_state.game_inst, (float)delta)) {
				KFATAL("Game render failed, shutting down.");
				app_state.is_running = 0;
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

			app_state.last_time = current_time;
		}
	}

	app_state.is_running = false;

	event_unregister(EVENT_CODE_APPLICATION_QUIT, 0, application_on_event);
	event_unregister(EVENT_CODE_KEY_PRESSED, 0, application_on_key);
	event_unregister(EVENT_CODE_KEY_RELEASED, 0, application_on_key);

	event_shutdown();
	input_shutdown();

	renderer_shutdown();

	platform_shutdown(&app_state.platform);

	return 1;
}

void application_get_framebuffer_size(uint32_t* width, uint32_t* height) {
	*width = app_state.width;
	*height = app_state.height;
}

uint8_t application_on_event(uint16_t code, void* sender, void* listener_inst, event_context context) {
	switch (code) {
		case EVENT_CODE_APPLICATION_QUIT: {
			KINFO("EVENT_CODE_APPLICATION_QUIT received, shutting down.\n");
			app_state.is_running = false;
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