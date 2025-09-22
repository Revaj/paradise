#pragma once
#include "core/application.h"


typedef struct game {
	application_config app_config;

	uint8_t (*initialize)(struct game* game_inst);

	uint8_t (*update)(struct game* game_inst, float delta_time);

	uint8_t (*render)(struct game* game_inst, float delta_time);

	void (*on_resize)(struct game* game_inst, uint32_t width, uint32_t height);

	void* state;

	void* application_state;
} game;