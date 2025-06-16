
#include "game_types.h"

typedef struct game_state {
	float delta_time;
} game_state;

uint8_t game_initialize(game* game_inst);

uint8_t game_update(game* game_inst, float delta_time);

uint8_t game_render(game* game_inst, float delta_time);

void game_on_resize(game* game_inst, uint32_t width, uint32_t height);