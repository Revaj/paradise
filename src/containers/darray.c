#include "darray.h"

#include "../core/gmemory.h"
#include "../core/logger.h"

void* _darray_create(uint64_t lenght, uint64_t stride) {
	uint64_t header_size = DARRAY_FIELD_LENGTH * sizeof(uint64_t);
	uint64_t array_size = lenght * stride;
	uint64_t* new_array = gallocate(header_size + array_size, MEMORY_TAG_DARRAY);
	gset_memory(new_array, 0, header_size + array_size);
	new_array[DARRAY_CAPACITY] = lenght;
	new_array[DARRAY_LENGTH] = 0;
	new_array[DARRAY_STRIDE] = stride;
	return (void*)(new_array + DARRAY_FIELD_LENGTH);
}

void _darray_destroy(void* array) {
	uint64_t* header = (uint64_t*)array - DARRAY_FIELD_LENGTH;
	uint64_t header_size = DARRAY_FIELD_LENGTH * sizeof(uint64_t);
	uint64_t total_size = header_size + header[DARRAY_CAPACITY] * header[DARRAY_STRIDE];
	gfree(header, total_size, MEMORY_TAG_DARRAY);
}

uint64_t _darray_field_get(void* array, uint64_t field) {
	uint64_t* header = (uint64_t*)array - DARRAY_FIELD_LENGTH;
	return header[field];
}

void _darray_field_set(void* array, uint64_t field, uint64_t value) {
	uint64_t* header = (uint64_t*)array - DARRAY_FIELD_LENGTH;
	header[field] = value;
}

void* _darray_resize(void* array) {
	uint64_t length = darray_length(array);
	uint64_t stride = darray_stride(array);
	void* temp = _darray_create(
		(DARRAY_RESIZE_FACTOR * darray_capacity(array)),
		stride);
	gcopy_memory(temp, array, length * stride);

	_darray_field_set(temp, DARRAY_LENGTH, length);
	_darray_destroy(array);
	return temp; 
}

void* _darray_push(void* array, const void* value_ptr) {
	uint64_t length = darray_length(array);
	uint64_t stride = darray_stride(array);
	if (length >= darray_capacity(array)) {
		array = _darray_resize(array);
	}

	uint64_t addr = (uint64_t)array;
	addr += (length * stride);
	gcopy_memory((void*)addr, value_ptr, stride);
	_darray_field_set(array, DARRAY_LENGTH, length + 1);
	return array;
}

void _darray_pop(void* array, void* dest) {
	uint64_t length = darray_length(array);
	uint64_t stride = darray_stride(array);
	uint64_t addr = (uint64_t)array;
	addr += ((length - 1) * stride);
	gcopy_memory(dest, (void*)addr, stride);
	_darray_field_set(array, DARRAY_LENGTH, length - 1);
}

void* _darray_pop_at(void* array, uint64_t index, void* dest) {
	uint64_t length = darray_length(array);
	uint64_t stride = darray_stride(array);
	if (index >= length) {
		KERROR("Index outside the bounds of this array! Length: %i, index: %index", length, index);
		return array;
	}

	uint64_t addr = (uint64_t)array;
	gcopy_memory(dest, (void*)(addr + (index * stride)), stride);

	if (index != length - 1) {
		gcopy_memory(
			(void*)(addr + (index * stride)),
			(void*)(addr + ((index + 1) * stride)),
			stride * (length - index));
		
	}

	_darray_field_set(array, DARRAY_LENGTH, length - 1);
	return array;
}

void* _darray_insert_at(void* array, uint64_t index, void* value_ptr) {
	uint64_t length = darray_length(array);
	uint64_t stride = darray_stride(array);
	if (index >= length) {
		KERROR("Index outside the bounds of this array! Length: %i, index: %index", length, index);
		return array;
	}
	if (length >= darray_capacity(array)) {
		array = _darray_resize(array);
	}

	uint64_t addr = (uint64_t)array;

	if (index != length - 1) {
		gcopy_memory(
			(void*)(addr + ((index + 1) * stride)),
			(void*)(addr + (index * stride)),
			stride * (length - index));
	}

	gcopy_memory((void*)(addr + (index * stride)), value_ptr, stride);

	_darray_field_set(array, DARRAY_LENGTH, length + 1);
	return array;
}