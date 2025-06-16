#pragma once

#include <stdlib.h>
#include <stdint.h>

typedef struct event_context {
	union {
		int64_t  i64[2];
		uint64_t u64[2];
		double   f64[2];

		int32_t  i32[4];
		uint32_t u32[4];
		float    f32[4];

		int16_t  i16[8];
		uint16_t u16[8];

		int8_t    i8[16];
		uint8_t   u8[16];

		char       c[16];
	} data;
} event_context;

typedef uint8_t(*PFN_on_event)(uint16_t code, void* sender, void* listener_inst, event_context data);

uint8_t event_initialize();
void event_shutdown();

uint8_t event_register(uint16_t code, void* listener, PFN_on_event on_event);

uint8_t event_unregister(uint16_t code, void* listener, PFN_on_event on_event);

uint8_t event_fire(uint16_t code, void* sender, event_context context);

typedef enum system_event_code {
	EVENT_CODE_APPLICATION_QUIT = 0x01,

	EVENT_CODE_KEY_PRESSED = 0x02,

	EVENT_CODE_KEY_RELEASED = 0x03,

	EVENT_CODE_BUTTON_PRESSED = 0x04,

	EVENT_CODE_BUTTON_RELEASED = 0x05,

	EVENT_CODE_MOUSE_MOVED = 0x06,

	EVENT_CODE_MOUSE_WHEEL = 0x07,

	EVENT_CODE_RESIZED = 0x08,

	MAX_EVENT_CODE = 0xFF,
} system_event_code;