# ft_ping

A basic reimplementation of the `ping` utility in C using raw ICMP sockets.

This project sends ICMP Echo Request packets to a host and measures round-trip time (RTT) from Echo Reply packets.

## Features

- Resolve a destination from DNS name or IPv4 address
- Send ICMP Echo Request packets through a raw socket (`AF_INET`, `SOCK_RAW`, `IPPROTO_ICMP`)
- Receive and filter ICMP Echo Reply packets
- Display per-packet output:
  - payload size
  - source IP
  - sequence number
  - reply TTL
  - RTT in ms
- Final summary:
  - transmitted / received packets
  - packet loss (%)
  - RTT min/avg/max/stddev
- Supported options:
  - `-v`: verbose header output (includes ICMP id)
  - `-c <number>`: stop after sending `<number>` packets
  - `-ttl <number>`: set outgoing packet TTL
  - `-h` or `-?`: display help

## Requirements

- Linux
- `gcc`
- `make`
- Permission to open raw sockets:
  - run as root, or
  - grant capability to the binary (`CAP_NET_RAW`)

## Build

```bash
make
```

Binary output:

```text
bin/ft_ping
```

## Usage

```bash
./bin/ft_ping [options] <destination>
```

Examples:

```bash
# Basic ping
./bin/ft_ping 8.8.8.8

# Ping by DNS name
./bin/ft_ping example.com

# Send exactly 5 packets
./bin/ft_ping -c 5 1.1.1.1

# Set TTL to 32
./bin/ft_ping -ttl 32 8.8.4.4

# Verbose mode
./bin/ft_ping -v -c 3 google.com
```

## How It Works

1. Parse CLI options and destination(s).
2. Resolve destination with.
3. Build an ICMP Echo Request packet:
   - type = `ICMP_ECHO`
   - id = current process id
   - sequence = incrementing counter
   - payload = simple incremental bytes
4. Compute ICMP checksum.
5. Send packet with `sendto`.
6. Receive packets with `recvfrom` and keep only matching replies.
7. Compute RTT from monotonic timestamps.
8. Print summary statistics when done (or on `Ctrl+C`).

## Math Behind the Statistics

### 1) ICMP Checksum (RFC 1071)

The checksum is the one's-complement of the one's-complement sum of all 16-bit words in the ICMP message:

$$
\text{checksum} = \sim\left(\sum_{i=1}^{n} w_i\right)
$$

Where carry bits are folded back into the low 16 bits.

### 2) Round-Trip Time (RTT)

For each reply:

$$
RTT_i = t_{recv,i} - t_{send,i}
$$

Displayed in milliseconds.

### 3) Average RTT

$$
\overline{RTT} = \frac{1}{N}\sum_{i=1}^{N} RTT_i
$$

Where $N$ is the number of received replies.

### 4) Packet Loss

If $T$ is transmitted packets and $R$ is received packets:

$$
\text{loss}(\%) = \frac{T - R}{T} \times 100
$$

### 5) RTT Standard Deviation

The implementation uses:

$$
\sigma = \sqrt{\frac{1}{N}\sum_{i=1}^{N} RTT_i^2 - \left(\overline{RTT}\right)^2}
$$

## Notes and Limitations

- Current implementation targets IPv4 (`sockaddr_in`).
- DNS resolution uses `gethostbyname`.
- Raw sockets require elevated privileges or `CAP_NET_RAW`.

## Cleaning

```bash
make clean   # remove object files
make fclean  # remove object files and binary
make re      # rebuild from scratch
```