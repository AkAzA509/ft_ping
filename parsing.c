#include "ft_ping.h"
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>

static void print_help(void)
{
	fprintf(stderr, "Usage\n  ping [options] <destination>\n\n");
	fprintf(stderr, "Options:\n");
	fprintf(stderr, "  <destination>  DNS name or IP address\n");
	fprintf(stderr, "  -v             verbose output\n");
	fprintf(stderr, "  -? -h          display help list\n");
	// fprintf(stderr, "  <destinantion> DNS name or IP address\n");
}

static bool parse_options(const char *opt, t_params *params)
{
	printf("opt = %s\n", opt);
	while(opt) {
		if (*opt == 'v')
			params->opts |= 0x01;
		else if (*opt == '?' || *opt == 'h') {
			printf("*opt = %c\n", *opt);
			print_help();
			return false;
		}
		opt++;
	}
	return true;
}

static bool parse_address(const char *addr, t_params *params)
{
	if (params->addr)
		return false;

	params->addr = strdup(addr);
	
	// parse the addr
	return true;
}

bool parse_params(char **av, t_params *params)
{
	while (*av) {
		if (**av == '-') {
			if (!parse_options(++*av, params))
				goto exit;
		}
		else if (isalpha(**av)) {
			if (!parse_address(*av, params))
				goto exit;
		}
		else {
			print_help();
			goto exit;
		}
		av++;
	}
	return true;

exit:
	return false;
}