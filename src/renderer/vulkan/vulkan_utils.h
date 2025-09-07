#pragma once
#include "vulkan_types.inl"

#include <stdbool.h>


const char* vulkan_result_string(VkResult result, int8_t get_extended);

bool vulkan_result_is_success(VkResult result);