#pragma once

#include <stdint.h>

typedef enum memory_tag {
	MEMORY_TAG_UNKNOWN,
	MEMORY_TAG_ARRAY,
	MEMORY_TAG_DARRAY,
	MEMORY_TAG_DICT,
	MEMORY_TAG_RING_QUEUE,
	MEMORY_TAG_BST,
	MEMORY_TAG_STRING,
	MEMORY_TAG_APPLICATION,
	MEMORY_TAG_JOB,
	MEMORY_TAG_TEXTURE,
	MEMORY_TAG_MATERIAL_INSTANCE,
	MEMORY_TAG_RENDERER,
	MEMORY_TAG_GAME,
	MEMORY_TAG_TRANSFORM,
	MEMORY_TAG_ENTITY,
	MEMORY_TAG_ENTITY_NODE,
	MEMORY_TAG_SCENE,

	MEMORY_TAG_MAX_TAGS
} memory_tag;

void initialize_memory();
void shutdown_memory();

void* gallocate(uint64_t size, memory_tag tag);
void gfree(void* block, uint64_t size, memory_tag tag);
void* gzero_memory(void* block, uint64_t size);
void* gcopy_memory(void* dest, const void* source, uint64_t size);
void* gset_memory(void* dest, int32_t value, uint64_t size);

char* get_memory_usage_str();