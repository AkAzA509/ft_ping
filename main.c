#include <errno.h>
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

#define PING_PKT_S 64
#define PING_DATA_S (PING_PKT_S - sizeof(struct icmphdr))
#define PING_SLEEP_RATE 1000000

t_params g_params = {
	.addr = NULL,
	.opts = 0,
	.addr_cap = 2,
	.c_val = -1,
	.ttl_val = 64,
};

struct packet {
	struct icmphdr hdr;
	char msg[PING_PKT_S - sizeof(struct icmphdr)];
};

bool run = true;
typedef long double ldbl;

void free_struct()
{
	if (g_params.addr) {
		for (size_t i = 0; g_params.addr[i]; ++i)
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
		clean_exit(BLD_RED "ft_ping: error unknow host\n" RESET, 1);

	char *ip = (char *)malloc(NI_MAXHOST * sizeof(char));
	if (!ip)
		clean_exit(BLD_RED "ft_ping: error malloc\n" RESET, 2);

	strcpy(ip, inet_ntoa(*(struct in_addr *)dns_addr->h_addr));
	(*addr_sock).sin_family = dns_addr->h_addrtype;
	(*addr_sock).sin_port = htons(0);
	(*addr_sock).sin_addr.s_addr = *(long *)dns_addr->h_addr;
	return ip;
}

// Calculate the checksum (RFC 1071)
unsigned short checksum(void *b, int len)
{
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

static int error_packet(struct sockaddr_in r_addr, int expected_seq,
	struct icmphdr *recv_hdr, const char *msg, ssize_t recv_len, int ip_hdr_len)
{
	unsigned char *embedded_ip =
		(unsigned char *)recv_hdr + sizeof(struct icmphdr);
	int embedded_ip_hdr_len = (embedded_ip[0] & 0x0F) * 4;
	if (recv_len < ip_hdr_len + (ssize_t)sizeof(struct icmphdr) + embedded_ip_hdr_len + (ssize_t)sizeof(struct icmphdr))
		return -1;
	struct icmphdr *embedded_icmp =
		(struct icmphdr *)(embedded_ip + embedded_ip_hdr_len);
	unsigned short orig_id = ntohs(embedded_icmp->un.echo.id);
	unsigned short orig_seq =
		ntohs(embedded_icmp->un.echo.sequence);
	if (orig_id != (unsigned short)getpid() ||
	    orig_seq != (unsigned short)expected_seq)
		return -2;

	printf(BLD_WHITE
	       "%d bytes from %s imcp_seq=%d %s\n" RESET,
	       PING_PKT_S, inet_ntoa(r_addr.sin_addr), orig_seq, msg);
	return -1;
}

static int check_packet(ssize_t recv_len, char *rbuffer, int expected_seq,
			unsigned short *recv_seq, struct sockaddr_in r_addr)
{
	int ip_hdr_len = (rbuffer[0] & 0x0F) * 4;
	if (recv_len < ip_hdr_len + (ssize_t)sizeof(struct icmphdr)) {
		fprintf(stderr,
			BLD_RED "Error... short packet received\n" RESET);
		return -1;
	}
	struct icmphdr *recv_hdr = (struct icmphdr *)(rbuffer + ip_hdr_len);
	unsigned short recv_id = ntohs(recv_hdr->un.echo.id);
	*recv_seq = ntohs(recv_hdr->un.echo.sequence);

	if (recv_hdr->type == ICMP_TIME_EXCEEDED)
		return error_packet(r_addr, expected_seq, recv_hdr,
			"Time to live exceeded", recv_len, ip_hdr_len);
	if (recv_hdr->type == ICMP_DEST_UNREACH)
		return error_packet(r_addr, expected_seq, recv_hdr,
			"Destination Host Unreachable", recv_len, ip_hdr_len);

	// filter our or misc packet
	if ((recv_hdr->type == ICMP_ECHO &&
	     recv_id == (unsigned short)getpid()) ||
	    (recv_hdr->type != ICMP_ECHOREPLY ||
	     recv_id != (unsigned short)getpid()) ||
	    (*recv_seq != (unsigned short)expected_seq))
		return -2;

	// else check its integrity
	if (checksum(recv_hdr, recv_len - ip_hdr_len) != 0) {
		fprintf(stderr, BLD_RED
			"Error ... packet integrity ckeck failed\n" RESET);
		return -1;
	}
	return 1;
}

static void receive_packet(int sock, int expected_seq,
			   struct timespec time_start, ldbl *sum_rtt_msec,
			   ldbl *sum_square_rtt_msec, ldbl *min,
			   ldbl *max, int *msg_received_count)
{
	struct sockaddr_in r_addr;
	char rbuffer[128];
	socklen_t addr_len = sizeof(r_addr);
	struct timespec time_end;
	unsigned short recv_seq = 0;

	while (1) {
		ssize_t recv_len = recvfrom(sock, rbuffer, sizeof(rbuffer), 0,
					    (struct sockaddr *)&r_addr,
					    &addr_len);
		if (recv_len <= 0) {
			fprintf(stderr,
				BLD_RED "Error ... packet receive failed!\n" RESET);
			return;
		}

		int ret = check_packet(recv_len, rbuffer, expected_seq,
				       &recv_seq, r_addr);
		if (ret == -1)
			return;
		else if (ret == -2)
			continue;
		else if (ret > 0)
			break;
	}
	clock_gettime(CLOCK_MONOTONIC, &time_end);
	unsigned char reply_ttl = (unsigned char)rbuffer[8];
	double elapsed =
		((double)(time_end.tv_nsec - time_start.tv_nsec)) / 1000000.0;
	ldbl rtt_msec =
		(time_end.tv_sec - time_start.tv_sec) * 1000.0 + elapsed;

	*sum_rtt_msec += rtt_msec;
	*sum_square_rtt_msec += rtt_msec * rtt_msec;
	printf(BLD_WHITE
	       "%d bytes from %s imcp_seq=%d ttl=%zu time=%.2Lf ms.\n" RESET,
	       PING_PKT_S, inet_ntoa(r_addr.sin_addr), recv_seq,
	       (size_t)reply_ttl, rtt_msec);
	*msg_received_count += 1;
	*max = rtt_msec > *max ? rtt_msec : *max;
	*min = rtt_msec < *min ? rtt_msec : *min;
}

static void print_stats(ldbl sum_rtt_msec, ldbl sum_square_rtt_msec, ldbl max,
			ldbl min, int msg_received_count, int sequence,
			ldbl stddev, char *domain)
{
	ldbl avg_rtt = 0.0L;
	if (msg_received_count == 0) {
		min = 0.0L;
		max = 0.0L;
		stddev = 0.0L;
	} else {
		avg_rtt = msg_received_count > 0 ?
				  sum_rtt_msec / msg_received_count :
				  0.0L;
		stddev = sqrtl((sum_square_rtt_msec / msg_received_count) -
			       (avg_rtt * avg_rtt));
	}
	printf(BLD_BLUE "\n--- %s ping statistics ---\n", domain);
	printf("%d packets transmitted, %d packets received, %.2f%% packet loss.\n",
	       sequence, msg_received_count,
	       ((sequence - msg_received_count) / (double)sequence) * 100.0);
	printf("round-trip min/avg/max/stddev = %.2Lf/%.2Lf/%.2Lf/%.2Lf ms\n\n" RESET,
	       min, avg_rtt, max, stddev);
}

static void prepare_packet(struct packet *pckt, int sequence)
{
	int i = 0;

	bzero(pckt, sizeof(*pckt));
	pckt->hdr.type = ICMP_ECHO;
	pckt->hdr.un.echo.id = htons((unsigned short)getpid());

	for (i = 0; (long unsigned int)i < sizeof(pckt->msg) - 1; i++)
		pckt->msg[i] = i + '0';

	pckt->msg[i] = 0;
	pckt->hdr.un.echo.sequence = htons((unsigned short)sequence);
	pckt->hdr.checksum = checksum(pckt, sizeof(*pckt));
}

static void send_packet(int sock, struct sockaddr_in *addr_sock, char *domain)
{
	ldbl sum_rtt_msec = 0, sum_square_rtt_msec = 0, max = LDBL_MIN,
	     min = LDBL_MAX, stddev = 0;
	int sequence = 0, msg_received_count = 0;
	struct timespec time_start;
	struct packet pckt;

	bool is_c = C_MASK(g_params.opts);
	size_t iteration = C_MASK(g_params.opts) ? g_params.c_val : 1;
	while (iteration && run) {
		if (is_c)
			iteration--;

		prepare_packet(&pckt, sequence);
		usleep(PING_SLEEP_RATE);

		clock_gettime(CLOCK_MONOTONIC, &time_start);
		if (sendto(sock, &pckt, sizeof(pckt), 0,
			   (struct sockaddr *)addr_sock,
			   sizeof(*addr_sock)) <= 0) {
			fprintf(stderr,
				BLD_RED "Error ... packet sending failed!\n" RESET);
			continue;
		}
		receive_packet(sock, sequence, time_start, &sum_rtt_msec,
			       &sum_square_rtt_msec, &min, &max,
			       &msg_received_count);
		sequence++;
	}
	print_stats(sum_rtt_msec, sum_square_rtt_msec, max, min,
		    msg_received_count, sequence, stddev, domain);
}

static int prepare_socket()
{
	int sock = socket(AF_INET, SOCK_RAW, IPPROTO_ICMP);
	if (sock < 0)
		return -1;

	struct timeval tv_out = {
		.tv_sec = 1,
		.tv_usec = 0,
	};

	if (setsockopt(sock, SOL_IP, IP_TTL, &g_params.ttl_val,
		       sizeof(g_params.ttl_val)) != 0) {
		fprintf(stderr, BLD_RED
			"Error ... setting socket options to TTL failed!\n" RESET);
		return -1;
	}

	setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, (const char *)&tv_out,
		   sizeof tv_out);
	return sock;
}

static void ping_loop()
{
	for (size_t i = 0; g_params.addr[i] && run; ++i) {
		struct sockaddr_in addr_sock;
		char *ip = dns_resolution(g_params.addr[i], &addr_sock);

		if (V_MASK(g_params.opts)) {
			unsigned short id = (unsigned short)getpid();
			printf(BLD_GREEN
			       "FT_PING %s (%s): %ld data bytes, id 0x%04x = %u\n" RESET,
			       g_params.addr[i], ip, PING_DATA_S, id, id);
		} else
			printf(BLD_GREEN
			       "FT_PING %s (%s): %ld data bytes\n" RESET,
			       g_params.addr[i], ip, PING_DATA_S);
		int sock = prepare_socket();
		if (sock < 0) {
			free(ip);
			fprintf(stderr, BLD_RED
				"ft_ping: error creation socket\n" RESET);
			continue;
		}
		send_packet(sock, &addr_sock, g_params.addr[i]);
		close(sock);
		free(ip);
	}
}

int main(int ac, char *av[])
{
	if (ac <= 1)
		clean_exit(
			BLD_RED
			"ft_ping: usage error: destination addresse required\n" RESET,
			1);

	parse_params(++av);
	signal(SIGINT, signint_handler);
	ping_loop();
	free_struct();
	return 0;
}