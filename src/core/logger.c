#include <stdarg.h>
#include <stdio.h>
#include <string.h>

#include "asserts.h"
#include "logger.h"
#include "../platform/platform.h"


void report_assertion_failure(const char* expression, const char* message, const char* file, int32_t line) {
	log_output(LOG_LEVEL_FATAL, "Assertion failure: %s, message: '%s', in file: %s, line: %d\n", expression, message, file, line);
}
bool initialize_logging() {
	return true;
}

void shutdown_logging() {

}


void log_output(log_level level, const char* message, ...) {
	const char* level_string[6] = { "[FATAL]: ", "[ERROR]: ", "[WARN]: ", "[INFO]: ", "[DEBUG]:", "[TRACE]: " };
	bool is_error = level < LOG_LEVEL_WARN;

	char out_message[32000];
	memset(out_message, 0, sizeof(out_message));

	va_list arg_ptr;
	va_start(arg_ptr, message);
	vsnprintf(out_message, 32000, message, arg_ptr);
	va_end(arg_ptr);

	char out_message2[32000];
	sprintf(out_message2, "%s %s\n", level_string[level], out_message);

	if (is_error) {
		platform_console_write_error(out_message2, level);
	}
	else {
		platform_console_write(out_message2, level);
	}
}