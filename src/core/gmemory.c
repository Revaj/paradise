#include "gmemory.h"
#include "logger.h"
#include "../platform/platform.h"

#include "gstring.h"
#include<string.h>
#include <stdio.h>

#include <stdlib.h>

struct memory_stats {
	uint64_t total_allocated;
	uint64_t tagged_allocations[MEMORY_TAG_MAX_TAGS];
};

static const char* memory_tag_strings[MEMORY_TAG_MAX_TAGS] = {
	"UNKNOWN    ",
	"ARRAY      ",
	"DARRAY     ",
	"DICT       ",
	"RING_QUEUE ",
	"BST        ",
	"STRING     ",
	"APPLICATION",
	"JOB        ",
	"TEXTURE    ",
	"MAT_INST   ",
	"RENDERER   ",
	"GAME       ",
	"TRANSFORM  ",
	"ENTITY     ",
	"ENTITY_NODE",
	"SCENE      ",
	"LINEAR_ALLOCATOR"};

typedef struct memory_system_state {
	struct memory_stats stats;
	uint64_t alloc_count;
}  memory_system_state;

static memory_system_state* state_ptr;

void memory_system_initialize(uint64_t* memory_requirement, void* state) {
	*memory_requirement = sizeof(memory_system_state);
	if (state == 0) {
		return;
	}

	state_ptr = state;
	state_ptr->alloc_count = 0;
	platform_zero_memory(&state_ptr->stats, sizeof(state_ptr->stats));
}

void memory_system_shutdown() {
	state_ptr = 0;

}

void* gallocate(uint64_t size, memory_tag tag) {
	if (tag == MEMORY_TAG_UNKNOWN) {
		KWARN("gallocate called using MEMORY_TAG_UNKNOWN. Re-class this allocation.");
	}

	if (state_ptr) {
		state_ptr->stats.total_allocated += size;
		state_ptr->stats.tagged_allocations[tag] += size;
	}

	void* block = platform_allocate(size, 0);
	if (block == NULL) {
		KWARN("aaaaaaaaaa %d", size);
	}
	platform_zero_memory(block, size);
	return block;
}

void gfree(void* block, uint64_t size, memory_tag tag) {
	if (tag == MEMORY_TAG_UNKNOWN) {
		KWARN("gfree called using MEMORY_TAG_UNKNOWN. Re-class this allocation.");
	}

	state_ptr->stats.total_allocated -= size;
	state_ptr->stats.tagged_allocations[tag] -= size;

	platform_free(block, 0);
}

void* gzero_memory(void* block, uint64_t size) {
	return platform_zero_memory(block, size);
}

void* gcopy_memory(void* dest, const void* source, uint64_t size) {
	return platform_copy_memory(dest, source, size);
}

void* gset_memory(void* dest, int32_t value, uint64_t size) {
	return platform_set_memory(dest, value, size);
}

char* get_memory_usage_str() {
	const uint64_t gib = 1024 * 1024 * 1024;
	const uint64_t mib = 1024 * 1024;
	const uint64_t kib = 1024;
	
	char buffer[8000] = "System memory use(tagged):\n";
	uint64_t offset = strlen(buffer);

	for (uint32_t i = 0; i < MEMORY_TAG_MAX_TAGS; ++i) {
		char unit[4] = "XiB";
		float amount = 1.0f;

		if (state_ptr->stats.tagged_allocations[i] >= gib) {
			unit[0] = 'G';
			amount = state_ptr->stats.tagged_allocations[i] / (float)gib;
		} else if (state_ptr->stats.tagged_allocations[i] >= mib) {
			unit[0] = 'M';
			amount = state_ptr->stats.tagged_allocations[i] / (float)mib;
		}
		else if (state_ptr->stats.tagged_allocations[i] >= kib) {
			unit[0] = 'K';
			amount = state_ptr->stats.tagged_allocations[i] / (float)kib;
		}
		else {
			unit[0] = 'B';
			unit[1] = 0;
			amount = (float)state_ptr->stats.tagged_allocations[i];
		}

		int32_t length = snprintf(buffer + offset, 8000, "  %s: %.2f%s\n", memory_tag_strings[i], amount, unit);
		offset += length;
	}
	char* out_string = string_duplicate(buffer);
	return out_string;
}