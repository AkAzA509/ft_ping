#include <signal.h>
#include <stddef.h>
#include <stdlib.h>
#include <stdio.h>
#include <netdb.h>
#include "ft_ping.h"

t_params g_params = {
	.addr = NULL,
	.opts = 0,
	.addr_cap = 2,
	.c_val = -1, // default value
};

void free_struct()
{
	if (g_params.addr) {
		for(size_t i = 0; g_params.addr[i]; ++i)
			free((void *)g_params.addr[i]);
		free((void *)g_params.addr);
	}
}

void clean_exit(const char *msg, int exit_code)
{
	fprintf(stderr, "%s", msg);
	free_struct();
	exit(exit_code);
}

// void d()
// {
// 	s = getaddrinfo(hostname, NULL, &hints, &result);
// 	if (s != 0) {
// 		fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(s));
// 		return EXIT_FAILURE;
// 	}
// }

void signint_handler(int sig)
{
	if (sig == SIGINT) {
		
		printf("exit");
	}
}

int main(int ac, char *av[])
{
	if (ac <= 1)
		clean_exit("ping: usage error: destination addresse required", 1);

	parse_params(++av);

	free_struct();
	return 0;
}