#include "ft_ping.h"
#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>

void print_tab(char *tab[])
{
	if (!tab)
		return ;
	for (size_t i = 0; tab[i]; ++i)
		printf("tab[%zu]: %s\n", i, tab[i]);
}

static void print_help(void)
{
	fprintf(stderr, "Usage\n  ft_ping [options] <destination>\n\n");
	fprintf(stderr, "Options:\n");
	fprintf(stderr, "  <destination>  DNS name or IP address\n");
	fprintf(stderr, "  -v             verbose output\n");
	fprintf(stderr, "  -? -h          display help list\n");
	fprintf(stderr, "  -c <number>    stop after sending <number> paquets\n");
}

static void parse_options(const char *opt, const char *next, bool *consumed)
{
	while(*opt) {
		if (*opt == 'v')
			g_params.opts |= 1 << 1;
		else if (*opt == 'c') {
			g_params.opts |= 1 << 2;
			if (!next || !isdigit((unsigned char)next[0]))
				clean_exit("ft_ping: missing value after -c option\n", 2);
			g_params.c_val = atoi(next);
			*consumed = true;
		}
		else if (*opt == '?' || *opt == 'h') {
			print_help();
			clean_exit("", 2);
		}
		opt++;
	}
}

static void parse_address(char *addr)
{
	size_t i = 0;

	if (!g_params.addr) {
		g_params.addr_cap = 2;
		g_params.addr = malloc(sizeof(char *) * g_params.addr_cap);
		if (!g_params.addr)
			clean_exit("ft_ping: error malloc\n", 2);
		memset(g_params.addr, 0, sizeof(char *) * g_params.addr_cap);
	}

	while (g_params.addr[i])
		i++;

	if (i + 1 >= g_params.addr_cap) {
		size_t new_cap = g_params.addr_cap * 2;
		void *tmp = realloc(g_params.addr, sizeof(char *) * new_cap);
		if (!tmp)
			clean_exit("ft_ping: error malloc\n", 2);

		g_params.addr = tmp;
		memset(g_params.addr + g_params.addr_cap, 0,
			sizeof(char *) * (new_cap - g_params.addr_cap));
		g_params.addr_cap = new_cap;
	}

	g_params.addr[i] = strdup(addr);
	if (!g_params.addr[i])
		clean_exit("ft_ping: error malloc\n", 2);
}

void parse_params(char *av[])
{
	bool consumed;

	while (*av) {
		if (*av && **av == '-') {
			parse_options(*av + 1, av[1], &consumed);
			if (consumed) {
				consumed = false;
				av++;
			}
		}
		else if (*av && isalpha(**av))
			parse_address(*av);
		else {
			print_help();
			clean_exit("", 2);
		}
		av++;
	}

	if (!g_params.addr)
		clean_exit("ft_ping: error no destination provided\n", 2);

	printf("params:\n");
	printf("\toptions: %c %s %zu\n", V_MASK(g_params.opts) ? 'v' : ' ',
								C_MASK(g_params.opts) ? "c =" : "",
								C_MASK(g_params.opts) ? g_params.c_val : 0);
	print_tab(g_params.addr);
}