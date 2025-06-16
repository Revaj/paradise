#include "game.h"

#include "core/logger.h"

uint8_t game_initialize(game* game_inst) {
	KDEBUG("game_initialize()");
	return 1;
}

uint8_t game_update(game* game_inst, float delta_time) {
	return 1;
}

uint8_t game_render(game* game_inst, float delta_time) {
	return 1;
}

void game_on_resize(game* game_inst, uint32_t width, uint32_t height) {

}