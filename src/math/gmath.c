#include "gmath.h"
#include "../platform/platform.h"

#include <stdlib.h>
#include <stdbool.h>
#include <math.h>

static bool rand_seeded = false;

float gsin(float x) {
	return sinf(x);
}

float gcos(float x) {
	return cosf(x);
}

float gtan(float x) {
	return tanf(x);
}

float gacos(float x) {
	return acosf(x);
}

float gsqrt(float x) {
	return sqrtf(x);
}

float gabs(float x) {
	return fabsf(x);
}

int32_t grandom() {
	if (!rand_seeded) {
		srand((uint32_t)platform_get_absolute_time());
		rand_seeded = true;
	}
	return rand();
}

int32_t grandom_in_range(int32_t min, int32_t max) {
	if (!rand_seeded) {
		srand((uint32_t)platform_get_absolute_time());
		rand_seeded = true;
	}

	return (rand() % (max - min + 1)) + min;
}

float fgrandom() {
	return (float)grandom() / (float)RAND_MAX;
}

float fgrandom_in_range(float min, float max) {
	return min + ((float)grandom() / ((float)RAND_MAX / (max - min)));
}