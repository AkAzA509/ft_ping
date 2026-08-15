#include <limits.h>
#include <stddef.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <netinet/ip.h>
#include <netinet/ip_icmp.h>
#include <time.h>
#include <fcntl.h>
#include <signal.h>
#include <float.h>
#include <math.h>
#include "ft_ping.h"

t_params g_params = {
	.addr = NULL,
	.opts = 0,
	.addr_cap = 2,
	.c_val = -1,
	.ttl_val = 64,
};
bool run = true;
#define PING_PKT_S 64
#define PING_DATA_S (PING_PKT_S - sizeof(struct icmphdr))
#define PING_SLEEP_RATE 1000000

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

void signint_handler(int sig)
{
	if (sig == SIGINT)
		run = !run;
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
	return ip;
}

struct packet {
	struct icmphdr hdr;
	char msg[PING_PKT_S - sizeof(struct icmphdr)];
};


// Calculate the checksum (RFC 1071)
unsigned short checksum(void *b, int len) {
	unsigned short *buf = b;
	unsigned int sum = 0;
	unsigned short result;

	for (sum = 0; len > 1; len -= 2)
		sum += *buf++;
	if (len == 1)
		sum += *(unsigned char *)buf;
	sum = (sum >> 16) + (sum & 0xFFFF);
	sum += (sum >> 16);
	result = ~sum;
	return result;
}

static void receive_packet(int sock, int *msg_count, struct timespec time_start,
	long double *sum_rtt_msec, long double *sum_square_rtt_msec, const char * ip,
	long double *min, long double *max, int *msg_received_count)
{
	struct sockaddr_in r_addr;
	char rbuffer[128];
	socklen_t addr_len = sizeof(r_addr);
	struct timespec time_end;
	long double rtt_msec = 0;

	ssize_t recv_len = recvfrom(sock, rbuffer, sizeof(rbuffer), 0, (struct sockaddr*)&r_addr, &addr_len);
	if (recv_len <= 0 && *msg_count > 1)
		fprintf(stderr, "\nPacket receive failed!\n");
	else {
		clock_gettime(CLOCK_MONOTONIC, &time_end);

		double elapsed = ((double)(time_end.tv_nsec - time_start.tv_nsec)) / 1000000.0;
		rtt_msec = (time_end.tv_sec - time_start.tv_sec) * 1000.0 + elapsed;
		*sum_rtt_msec += rtt_msec;
		*sum_square_rtt_msec += rtt_msec * rtt_msec;
		int ip_hdr_len = (rbuffer[0] & 0x0F) * 4;
		if (recv_len < ip_hdr_len + (ssize_t)sizeof(struct icmphdr)) {
			fprintf(stderr, "Error... short packet received\n");
			return;
		}

		unsigned char reply_ttl = (unsigned char)rbuffer[8];
		struct icmphdr *recv_hdr = (struct icmphdr *)(rbuffer + ip_hdr_len);
		if (!(recv_hdr->type == 0 && recv_hdr->code == 0))
			fprintf(stderr, "Error... Packet received with ICMP type %d code %d\n", recv_hdr->type, recv_hdr->code);
		else {
			printf("%d bytes from %s imcp_seq=%d ttl=%zu time=%.2Lf ms.\n",
				PING_PKT_S, ip, *msg_count, (size_t)reply_ttl, rtt_msec);
			*msg_received_count += 1;
			*max = rtt_msec > *max ? rtt_msec : *max;
			*min = rtt_msec < *min ? rtt_msec : *min;
		}
	}
}

static void send_packet(int sock, struct sockaddr_in *addr_sock, char *ip, char *domain)
{
	int msg_count = 0, i, msg_received_count = 0;
	struct packet pckt;
	struct timespec time_start;
	long double sum_rtt_msec = 0, sum_square_rtt_msec = 0, max = LDBL_MIN, min = LDBL_MAX, stddev = 0;
	struct timeval tv_out;
	tv_out.tv_sec = 1;
	tv_out.tv_usec = 0;

	// Set socket options at IP to TTL and value to 64
	if (setsockopt(sock, SOL_IP, IP_TTL, &g_params.ttl_val, sizeof(g_params.ttl_val)) != 0) {
		fprintf(stderr, "\nSetting socket options to TTL failed!\n");
		return;
	}

	// Setting timeout of receive setting
	setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, (const char*)&tv_out, sizeof tv_out);

	bool is_c = C_MASK(g_params.opts);
	size_t iteration = C_MASK(g_params.opts) ? g_params.c_val : 1;
	while (iteration) {
		if (is_c)
			iteration--;
		if (!run)
			break;
		bzero(&pckt, sizeof(pckt));
		pckt.hdr.type = ICMP_ECHO;
		pckt.hdr.un.echo.id = getpid();

		for (i = 0; (long unsigned int)i < sizeof(pckt.msg) - 1; i++)
			pckt.msg[i] = i + '0';

		pckt.msg[i] = 0;
		pckt.hdr.un.echo.sequence = msg_count++;
		pckt.hdr.checksum = checksum(&pckt, sizeof(pckt));

		usleep(PING_SLEEP_RATE);
		// Send packet
		clock_gettime(CLOCK_MONOTONIC, &time_start);
		if (sendto(sock, &pckt, sizeof(pckt), 0, (struct sockaddr*)addr_sock, sizeof(*addr_sock)) <= 0) {
			fprintf(stderr, "\nPacket Sending Failed!\n");
			continue;
		}
		// Receive packet
		receive_packet(sock, &msg_count, time_start, &sum_rtt_msec, &sum_square_rtt_msec, ip, &min, &max, &msg_received_count);
	}
	long double avg_rtt = msg_received_count > 0 ? sum_rtt_msec / msg_received_count : 0.0L;
	stddev = sqrtl((sum_square_rtt_msec / msg_received_count) - (avg_rtt * avg_rtt));
	printf("\n--- %s ping statistics ---\n", domain);
	printf("%d packets transmitted, %d packets received, %.2f%% packet loss.\n",
		msg_count, msg_received_count, ((msg_count - msg_received_count) / (double)msg_count) * 100.0);
	printf("round-trip min/avg/max/stddev = %.2Lf/%.2Lf/%.2Lf/%.2Lf\n", min, avg_rtt, max, stddev);
	return;
}

static void ping_loop()
{
	for (size_t i = 0; g_params.addr[i]; ++i) {
		struct sockaddr_in addr_sock;
		char *ip = dns_resolution(g_params.addr[i], &addr_sock);
		printf("ft_ping %s (%s): %ld data bytes\n", g_params.addr[i], ip, PING_DATA_S);
		int sock = socket(AF_INET, SOCK_RAW, IPPROTO_ICMP);
		if (sock < 0) {
			free(ip);
			fprintf(stderr, "ft_ping: error creation socket\n");
			continue;
		}
		send_packet(sock, &addr_sock, ip, g_params.addr[i]);
		close(sock);
		free(ip);
	}
}

int main(int ac, char *av[])
{
	if (ac <= 1)
		clean_exit("ping: usage error: destination addresse required\n", 1);

	parse_params(++av);
	signal(SIGINT, signint_handler);
	ping_loop();
	free_struct();
	return 0;
}
