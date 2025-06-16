#pragma once
#include <stdint.h>

struct game;

typedef struct application_config {
	int16_t start_pos_x;
	int16_t start_pos_y;

	int16_t start_width;
	int16_t start_height;
	char* name;
} application_config;

uint8_t application_create(struct game* game_inst);

uint8_t application_run();