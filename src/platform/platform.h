#include <stdint.h>

typedef struct platform_state {
	void* internal_state;
} platform_state;

uint8_t platform_startup(platform_state* plat_state, const char* application_name, int32_t x, int32_t y, int32_t width, int32_t height);

void platform_shutdown(platform_state* plat_state);

uint8_t platform_pump_messages(platform_state* plat_state);

void* platform_allocate(uint64_t size, uint8_t align);
void platform_free(void* block, uint8_t align);
void* platform_zero_memory(void* block, uint64_t size);
void* platform_copy_memory(void* dest, const void* source, uint64_t size);
void* platform_set_memory(void* dest, int32_t value, uint64_t size);

void platform_console_write(const char* message, uint8_t colour);
void platform_console_write_error(const char* message, uint8_t colour);

double platform_get_absolute_time();

void platform_sleep(uint64_t ms);