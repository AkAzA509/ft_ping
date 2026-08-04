#ifndef FT_PING_H
#define FT_PING_H

#include <stdint.h>
#include <stdbool.h>

#define V_MASK(x) ((x) & 0xFF)

typedef struct {
	uint8_t opts;
	const char *addr;
} t_params;

bool parse_params(char *av[], t_params *params);

#endif // FT_PING_H