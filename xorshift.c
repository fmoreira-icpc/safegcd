#define _CRT_RAND_S

#include <stdint.h>
#include <stdlib.h>

static uint64_t seed;

void XS_init() {
	do {
		unsigned int u, v;
		rand_s(&u); rand_s(&v);
		seed = ((uint64_t) u << 32) | ((uint64_t) v << 0);
	} while (seed == 0);
}

uint64_t XS_next() {
	seed ^= seed << 13;
	seed ^= seed >> 7;
	seed ^= seed << 17;
	return seed;
}