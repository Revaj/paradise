#pragma once
#include <stdbool.h>

#define LOG_WARN_ENABLED 1
#define LOG_INFO_ENABLED 1
#define LOG_DEBUG_ENABLED 1
#define LOG_TRACE_ENABLED 1

#if PRELEASE == 1
#define LOG_DEBUG_ENABLED 0
#define LOG_TRACE_ENABLED 0
#endif

typedef enum log_level {
	LOG_LEVEL_FATAL = 0,
	LOG_LEVEL_ERROR = 1,
	LOG_LEVEL_WARN = 2,
	LOG_LEVEL_INFO = 3,
	LOG_LEVEL_DEBUG = 4,
	LOG_LEVEL_TRACE = 5
} log_level;

bool initialize_logging();
void shutdown_logging();

extern void log_output(log_level level, const char* message, ...);

#define KFATAL(message, ...) log_output(LOG_LEVEL_FATAL, message, ##__VA_ARGS__);

#ifndef KERROR
#define KERROR(message, ...) log_output(LOG_LEVEL_ERROR, message, ##__VA_ARGS__);
#endif

#if LOG_WARN_ENABLED == 1
#define KWARN(message, ...) log_output(LOG_LEVEL_WARN, message, ##__VA_ARGS__);
#else
#define KWARN(message, ...)
#endif

#if LOG_INFO_ENABLED == 1
#define KINFO(message, ...) log_output(LOG_LEVEL_INFO, message, ##__VA_ARGS__);
#else
#define KINFO(message, ...)
#endif

#if LOG_DEBUG_ENABLED == 1
#define KDEBUG(message, ...) log_output(LOG_LEVEL_DEBUG, message, ##__VA_ARGS__);
#else
#define KDEBUG(message, ...)
#endif

#if LOG_TRACE_ENABLED == 1
#define KTRACE(message, ...) log_output(LOG_LEVEL_TRACE, message, ##__VA_ARGS__);
#else
#define KTRACE(message, ...)
#endif