#ifndef FT_PING_H
#define FT_PING_H

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

#define GREEN "\033[92m"
#define WHITE "\033[97m"
#define BLUE "\033[94m"
#define RED "\033[91m"
#define PURPLE "\033[38;2;255;105;255m"
#define BOLD "\033[1m"
#define RESET "\033[0m"

#define BLD_WHITE BOLD WHITE
#define BLD_RED BOLD RED
#define BLD_BLUE BOLD BLUE
#define BLD_GREEN BOLD GREEN
#define BLD_PURPLE BOLD PURPLE

#define V_MASK(x) ((x) & 1 << 1)
#define C_MASK(x) ((x) & 1 << 2)
#define TTL_MASK(x) ((x) & 1 << 3)

typedef struct {
	uint8_t opts;
	size_t c_val;
	size_t ttl_val;
	char **addr;
	size_t addr_cap;
} t_params;

extern t_params g_params;

void clean_exit(const char *msg, int exit_code);
void parse_params(char *av[]);

#endif // FT_PING_H
