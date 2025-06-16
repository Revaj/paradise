// paradise.cpp: define el punto de entrada de la aplicación.
//

#include "src/core/logger.h"
#include "src/entry.h"
#include "src/game.h"

#include "src/core/gmemory.h"
uint8_t create_game(game* out_game) {
	out_game->app_config.start_pos_x = 100;
	out_game->app_config.start_pos_y = 100;
	out_game->app_config.start_width = 1200;
	out_game->app_config.start_height = 720;
	out_game->app_config.name = "Yavi engine";

	out_game->update = game_update;
	out_game->render = game_render;
	out_game->initialize = game_initialize;
	out_game->on_resize = game_on_resize;

	out_game->state = gallocate(sizeof(game_state), MEMORY_TAG_GAME);

	return 1;
}

