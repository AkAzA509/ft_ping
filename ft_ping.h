#ifndef FT_PING_H
#define FT_PING_H

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

#define V_MASK(x) ((x) & 1 << 1)
#define C_MASK(x) ((x) & 1 << 2)

typedef struct {
	uint8_t opts;
	size_t c_val;
	char **addr;
	size_t addr_cap;
} t_params;

extern t_params g_params;

void clean_exit(const char *msg, int exit_code);
void parse_params(char *av[]);

#endif // FT_PING_H
