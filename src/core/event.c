#include "event.h"
#include "gmemory.h"
#include "../containers/darray.h"


typedef struct registered_event {
	void* listener;
	PFN_on_event callback;
} registered_event;

typedef struct event_code_entry {
	registered_event* events;
} event_code_entry;

#define MAX_MESSAGE_CODES 16384

typedef struct event_system_state {
	event_code_entry registered[MAX_MESSAGE_CODES];
} event_system_state;

//EVENTS internal state check how to move this please is fucking ugly
static uint8_t is_initialized = 0;
static event_system_state* state_ptr;

void event_system_initialize(uint64_t* memory_requirement, void* state) {
	*memory_requirement = sizeof(event_system_state);
	if (state == 0) {
		return;
	}

	gzero_memory(state, sizeof(state));
	state_ptr = state;
}

void event_system_shutdown(void* state) {
	if (state_ptr) {
		for (uint16_t i = 0; i < MAX_MESSAGE_CODES; ++i) {
			if (state_ptr->registered[i].events != 0) {
				darray_destroy(state_ptr->registered[i].events);
				state_ptr->registered[i].events = 0;
			}
		}
	}
	state_ptr = 0;
}

uint8_t event_register(uint16_t code, void* listener, PFN_on_event on_event) {
	if (!state_ptr) {
		return 0;
	}

	if (state_ptr->registered[code].events == 0) {
		state_ptr->registered[code].events = darray_create(registered_event);
	}

	uint64_t registered_count = darray_length(state_ptr->registered[code].events);
	for (uint64_t i = 0; i < registered_count; ++i) {
		if (state_ptr->registered[code].events[i].listener == listener) {
			return 0;
		}
	}

	registered_event event;
	event.listener = listener;
	event.callback = on_event;
	_darray_push(state_ptr->registered[code].events, &event);

	return 1;
}

uint8_t event_unregister(uint16_t code, void* listener, PFN_on_event on_event) {
	if (!state_ptr) {
		return 0;
	}

	if (state_ptr->registered[code].events == 0) {
		state_ptr->registered[code].events = darray_create(registered_event);
	}

	uint64_t registered_count = darray_length(state_ptr->registered[code].events);
	for (uint64_t i = 0; i < registered_count; ++i) {
		registered_event e = state_ptr->registered[code].events[i];
		if (e.listener == listener && e.callback == on_event) {
			registered_event popped_event;
			darray_pop_at(state_ptr->registered[code].events, i, &popped_event);
			return 1;
		}
	}

	return 0;
}

uint8_t event_fire(uint16_t code, void* sender, event_context context) {
	if (!state_ptr) {
		return 0;
	}

	if (state_ptr->registered[code].events == 0) {
		return 0;
	}


	uint64_t registered_count = darray_length(state_ptr->registered[code].events);
	for (uint64_t i = 0; i < registered_count; ++i) {
		registered_event e = state_ptr->registered[code].events[i];
		if (e.callback(code, sender, e.listener, context)) {

			return 1;
		}
	}

	return 0;
}