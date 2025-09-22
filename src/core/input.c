#include "input.h"
#include "event.h"
#include "gmemory.h"
#include "logger.h"


typedef struct keyboard_state {
	uint8_t keys[256];
} keyboard_state;

typedef struct mouse_state {
	int16_t x;
	int16_t y;
	uint8_t buttons[BUTTON_MAX_BUTTONS];
} mouse_state;

typedef struct input_state {
	keyboard_state keyboard_current;
	keyboard_state keyboard_previous;
	mouse_state mouse_current;
	mouse_state mouse_previous;
} input_state;

static input_state *state_ptr;

void input_system_initialize(uint64_t* memory_requirement, void* state) {
	*memory_requirement = sizeof(input_state);
	if (state == 0) {
		return;
	}
	gzero_memory(state, sizeof(input_state));
	state_ptr = state;
}

void input_system_shutdown() {
	state_ptr = 0;
}

void input_update(double delta_time) {
	if (!state_ptr) {
		return;
	}
	gcopy_memory(&state_ptr->keyboard_previous, &state_ptr->keyboard_current, sizeof(keyboard_state));
	gcopy_memory(&state_ptr->mouse_previous, &state_ptr->mouse_current, sizeof(mouse_state));
}

void input_process_key(keys key, uint8_t pressed) {
	if (state_ptr->keyboard_current.keys[key] != pressed) {
		state_ptr->keyboard_current.keys[key] = pressed;

		event_context context;
		context.data.u16[0] = key;
		event_fire(pressed ? EVENT_CODE_KEY_PRESSED : EVENT_CODE_KEY_RELEASED, 0, context);
	}
}

void input_process_button(buttons button, uint8_t pressed) {
	if (state_ptr->mouse_current.buttons[button] != pressed) {
		state_ptr->mouse_current.buttons[button] = pressed;

		event_context context;
		context.data.u16[0] = button;
		event_fire(pressed ? EVENT_CODE_BUTTON_PRESSED : EVENT_CODE_BUTTON_RELEASED, 0, context);
	}
}

void input_process_mouse_move(int16_t x, int16_t y) {
	if (state_ptr->mouse_current.x != x || state_ptr->mouse_current.y != y) {

		state_ptr->mouse_current.x = x;
		state_ptr->mouse_current.y = y;

		event_context context;
		context.data.u16[0] = x;
		context.data.u16[1] = y;
		event_fire(EVENT_CODE_MOUSE_MOVED, 0, context);
	}
}

void input_process_mouse_wheel(int8_t z_delta) {
	event_context context;
	context.data.u8[0] = z_delta;
	event_fire(EVENT_CODE_MOUSE_WHEEL, 0, context);
}

bool input_is_key_down(keys key) {
	if (!state_ptr) {
		return false;
	}

	return state_ptr->keyboard_current.keys[key] == 1;
}

bool input_is_key_up(keys key) {
	if (!state_ptr) {
		return false;
	}

	return state_ptr->keyboard_current.keys[key] == 0;
}

bool input_was_key_down(keys key) {
	if (!state_ptr) {
		return 0;
	}

	return state_ptr->keyboard_previous.keys[key] == 1;
}

bool input_was_key_up(keys key) {
	if (!state_ptr) {
		return false;
	}

	return state_ptr->keyboard_previous.keys[key] == 0;
}

//mouse

bool input_is_button_down(buttons button) {
	if (!state_ptr) {
		return false;
	}

	return state_ptr->mouse_current.buttons[button] == 1;
}

bool input_is_button_up(buttons button) {
	if (!state_ptr) {
		return false;
	}

	return state_ptr->mouse_current.buttons[button] == 0;
}

bool input_was_button_down(buttons button) {
	if (!state_ptr) {
		return false;
	}

	return state_ptr->mouse_previous.buttons[button] == 1;
}

bool input_was_button_up(buttons button) {
	if (!state_ptr) {
		return false;
	}

	return state_ptr->mouse_previous.buttons[button] == 0;
}

void input_get_mouse_position(int32_t* x, int32_t* y) {
	if (!state_ptr) {
		*x = 0;
		*y = 0;
		return;
	}
	*x = state_ptr->mouse_current.x;
	*y = state_ptr->mouse_current.y;
}

void input_get_previous_mouse_position(int32_t* x, int32_t* y) {
	if (!state_ptr) {
		*x = 0;
		*y = 0;
		return;
	}
	*x = state_ptr->mouse_previous.x;
	*y = state_ptr->mouse_previous.y;
}


