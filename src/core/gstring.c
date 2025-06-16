#include "gstring.h"
#include "gmemory.h"

#include <string.h>


uint64_t string_length(const char* str) {
    return strlen(str);
}

char* string_duplicate(const char* str) {
    uint64_t length = string_length(str);
    char* copy = gallocate(length + 1, MEMORY_TAG_STRING);
    gcopy_memory(copy, str, length + 1);
    return copy;
}