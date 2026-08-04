#include <stdlib.h>
#include <stdio.h>
#include "ft_ping.h"

void free_struct(t_params params)
{
	if (params.addr)
		free((void *)params.addr);
}

int main(int ac, char *av[])
{
	if (ac <= 1) {
		fprintf(stderr, "ping: usage error: destination addresse required\n");
		return 1;
	}

	t_params params = {
		.addr = 0,
		.opts = 0,
	};
	if (!parse_params(++av, &params)) {
		free_struct(params);
		return 2;
	}

	free_struct(params);
	return 0;
}