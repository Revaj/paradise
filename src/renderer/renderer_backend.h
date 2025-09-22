#pragma once

#include <stdbool.h>
#include "renderer_types.inl"

struct platform_state;

bool renderer_backend_create(renderer_backend_type type,
							 renderer_backend* out_renderer_backend);

void renderer_backend_destroy(renderer_backend* renderer_backend);