﻿# CMakeList.txt: proyecto de CMake para paradise, incluya el origen y defina
# la lógica específica del proyecto aquí.
#

# Agregue un origen al ejecutable de este proyecto.
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_C_STANDARD 23)

add_executable (paradise "paradise.c")
target_sources(
	paradise
	PUBLIC
	src/core/logger.h
	src/core/logger.c
	src/core/asserts.h
	src/core/application.h
	src/core/application.c
	src/platform/platform.h
	src/platform/platform_win32.c
	src/platform/platform_linux.c

	src/entry.h
	src/game.c
	src/game.h
	src/game_types.h

	src/containers/darray.c
	src/containers/darray.h

	src/core/gmemory.c
	src/core/gmemory.h

	src/core/event.c
	src/core/event.h

	src/core/input.c
	src/core/input.h

	src/core/gstring.c
	src/core/gstring.h
)

#comment if it's windows
target_link_libraries(paradise xcb X11 X11-xcb xkbcommon)

if (CMAKE_VERSION VERSION_GREATER 3.12)
  set_property(TARGET paradise PROPERTY  C_STANDARD 23)
endif()


# TODO: Agregue pruebas y destinos de instalación si es necesario.
