#pragma once
#include <stdbool.h>
#include <stdint.h>

typedef struct linear_allocator {
    uint64_t total_size;
    uint64_t allocated;
    void* memory;
    bool owns_memory;
} linear_allocator;

void linear_allocator_create(uint64_t total_size, void* memory, linear_allocator* out_allocator);
void linear_allocator_destroy(linear_allocator* allocator);

void* linear_allocator_allocate(linear_allocator* allocator, uint64_t size);
void linear_allocator_free_all(linear_allocator* allocator);