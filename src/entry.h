#include "core/application.h"
#include "core/logger.h"
#include "core/gmemory.h"
#include "game_types.h"

extern uint8_t create_game(game* out_game);

int main()
{
	game game_inst;

	if (!create_game(&game_inst)) {
		KFATAL("Could not create game!");
		return -1;
	}

	if (!game_inst.render || !game_inst.update || !game_inst.initialize || !game_inst.on_resize) {
		KFATAL("The game's function pointers must be assigned!");
		return -2;
	}

	if(!application_create(&game_inst)) {
		KINFO("Appliation failed to create!\n");
		return -3;
	}

	if (!application_run()) {
		KINFO("Application did not shutdown gracefully.");
		return -3;
	}

	memory_system_shutdown();
	return 0;
}