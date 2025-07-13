
cmake_minimum_required(VERSION 3.28)
project(Paradise)

    if(COMMAND cmake_policy)
      cmake_policy(SET CMP0003 NEW)
    endif(COMMAND cmake_policy)


set(CMAKE_CXX_STANDARD 20)

find_package(Vulkan REQUIRED)
find_program(WAYLAND_SCANNER_EXECUTABLE NAMES wayland-scanner)
if (NOT WAYLAND_SCANNER_EXECUTABLE)
    message(FATAL_ERROR "Failed to find wayland-scanner")
endif()


if (WIN32)
	add_compile_options(/std:clatest)
endif()

if (CMAKE_BUILD_TYPE MATCHES Debug)
	add_definitions(-D_DEBUG)
endif()

	add_compile_options(-g)


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

	src/core/clock.h
	src/core/clock.c

	src/renderer/renderer_backend.c
	src/renderer/renderer_backend.h
	src/renderer/renderer_frontend.c
	src/renderer/renderer_frontend.h
	src/renderer/renderer_types.inl

	src/renderer/vulkan/vulkan_backend.c
	src/renderer/vulkan/vulkan_backend.h
	src/renderer/vulkan/vulkan_types.inl
	src/renderer/vulkan/vulkan_platform.h
)

#comment if it's windows
if(NOT WIN32)
	add_custom_command(OUTPUT "${CMAKE_SOURCE_DIR}/src/platform/protocol.h"
    COMMAND "${WAYLAND_SCANNER_EXECUTABLE}" client-header "/usr/share/wayland-protocols/stable/xdg-shell/xdg-shell.xml" "${CMAKE_SOURCE_DIR}/src/platform/protocol.h"
    DEPENDS "/usr/share/wayland-protocols/stable/xdg-shell/xdg-shell.xml"
    VERBATIM)

	add_custom_command(OUTPUT "protocol.c"
    COMMAND "${WAYLAND_SCANNER_EXECUTABLE}" private-code "/usr/share/wayland-protocols/stable/xdg-shell/xdg-shell.xml" "protocol.c"
    DEPENDS "/usr/share/wayland-protocols/stable/xdg-shell/xdg-shell.xml"
    VERBATIM)

	target_sources(paradise PRIVATE protocol.c src/platform/protocol.h)
	target_link_libraries(paradise xcb X11 X11-xcb xkbcommon wayland-client)
endif()

target_link_libraries(paradise  Vulkan::Vulkan)

if (CMAKE_VERSION VERSION_GREATER 3.12)
  set_property(TARGET paradise PROPERTY  C_STANDARD 23)
endif()


# TODO: Agregue pruebas y destinos de instalación si es necesario.
