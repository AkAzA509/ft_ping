#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <netinet/ip_icmp.h>
#include <time.h>
#include <fcntl.h>
#include <signal.h>
#include "ft_ping.h"

t_params g_params = {
	.addr = NULL,
	.opts = 0,
	.addr_cap = 2,
	.c_val = -1,
	.ttl_val = 64,
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

static void print_stat()
{
	printf("coucou\n");
}

void signint_handler(int sig)
{
	if (sig == SIGINT) {
		print_stat();
		printf("exit");
	}
}

static char *dns_resolution(const char *addr, struct sockaddr_in *addr_sock)
{
	struct hostent *dns_addr = gethostbyname(addr);
	if (!dns_addr)
		clean_exit("ft_ping: error unknow host\n", 1);

	char *ip = (char *)malloc(NI_MAXHOST * sizeof(char));
	if (!ip)
		clean_exit("ft_ping: error malloc\n", 2);

	strcpy(ip, inet_ntoa(*(struct in_addr *)dns_addr->h_addr));
	(*addr_sock).sin_family = dns_addr->h_addrtype;
	(*addr_sock).sin_port = htons(0);
	(*addr_sock).sin_addr.s_addr = *(long *)dns_addr->h_addr;

	printf("ip: %s\n", ip);
	return ip;
}

static void send_packet(int sock, struct sockaddr_in *addr_sock, char *ip, char *domain)
{
	(void)sock;
	(void)addr_sock;
	(void)ip;
	(void)domain;
}

int main(int ac, char *av[])
{
	if (ac <= 1)
		clean_exit("ping: usage error: destination addresse required\n", 1);
	
	parse_params(++av);
	signal(SIGINT, signint_handler);

	for (size_t i = 0; g_params.addr[i]; ++i) {
		struct sockaddr_in addr_sock;
		char *ip = dns_resolution(g_params.addr[i], &addr_sock);
		int sock = socket(AF_INET, SOCK_RAW, IPPROTO_ICMP);
		if (sock < 0) {
			free(ip);
			fprintf(stderr, "ft_ping: error creation socket\n");
			continue;
		}
		send_packet(sock, &addr_sock, ip, g_params.addr[i]);
		// received_packet();
		close(sock);
		free(ip);
	}

	free_struct();
	return 0;
}