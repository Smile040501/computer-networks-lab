// Send an IPv4 UDP packet via raw socket at the link layer (ethernet frame).
// Need to have destination MAC address.
// Includes some UDP data.

#include <arpa/inet.h>        // inet_pton() and inet_ntop()
#include <errno.h>            // errno, perror()
#include <linux/if_ether.h>   // ETH_P_IP = 0x0800, ETH_P_IPV6 = 0x86DD
#include <linux/if_packet.h>  // struct sockaddr_ll (see man 7 packet)
#include <net/if.h>           // struct ifreq
#include <netdb.h>            // struct addrinfo
#include <netinet/in.h>       // IPPROTO_RAW, IPPROTO_IP, IPPROTO_UDP, INET_ADDRSTRLEN
#include <netinet/ip.h>       // struct ip and IP_MAXPACKET (which is 65535)
#include <netinet/udp.h>      // struct udphdr
#include <string.h>           // strcpy, memset(), and memcpy()
#include <sys/ioctl.h>        // macro ioctl is defined
#include <sys/socket.h>       // needed for socket()
#include <sys/types.h>        // needed for socket(), uint8_t, uint16_t, uint32_t
#include <unistd.h>           // close()

#include <cstring>
#include <iomanip>
#include <iostream>
using namespace std;

// Define some constants.
#define ETH_HDRLEN 14  // Ethernet header length
#define IP4_HDRLEN 20  // IPv4 header length
#define UDP_HDRLEN 8   // UDP header length, excludes data

// Function prototypes
uint16_t checksum(uint16_t *, int);
uint16_t udp4_checksum(struct ip, struct udphdr, uint8_t *, int);
char *allocate_strmem(int);
uint8_t *allocate_ustrmem(int);
int *allocate_intmem(int);

void printMACAddress(unsigned char *addr) {
    ios init(NULL);
    // copying the default `cout` formatting
    init.copyfmt(std::cout);

    for (int i = 0; i < ETH_ALEN; ++i) {
        std::cout << std::setfill('0') << std::setw(2) << std::hex << static_cast<unsigned int>(addr[i]);
        if (i != ETH_ALEN - 1) std::cout << ':';
    }

    // resetting the default `cout` formatting
    std::cout.copyfmt(init);
    std::cout << '\n';
}

int main(int argc, char **argv) {
    int i, status, datalen, frame_length, sd, bytes, *ip_flags;
    char *interface, *target, *src_ip, *dst_ip;
    struct ip iphdr;
    struct udphdr udphdr;
    uint8_t *data, *src_mac, *dst_mac, *ether_frame;
    struct addrinfo hints, *res;
    struct sockaddr_in *ipv4;
    struct sockaddr_ll device;
    struct ifreq ifr;
    void *tmp;

    // Allocate memory for various arrays.
    src_mac = allocate_ustrmem(6);
    dst_mac = allocate_ustrmem(6);
    data = allocate_ustrmem(IP_MAXPACKET);
    ether_frame = allocate_ustrmem(IP_MAXPACKET);
    interface = allocate_strmem(40);
    target = allocate_strmem(40);
    src_ip = allocate_strmem(INET_ADDRSTRLEN);
    dst_ip = allocate_strmem(INET_ADDRSTRLEN);
    ip_flags = allocate_intmem(4);

    // Interface to send packet through.
    strcpy(interface, "eth0");

    // Submit request for a socket descriptor to look up interface.
    if ((sd = socket(PF_PACKET, SOCK_RAW, htons(ETH_P_ALL))) < 0) {
        perror("socket() failed to get socket descriptor for using ioctl() ");
        exit(EXIT_FAILURE);
    }

    // Use ioctl() to look up interface name and get its MAC address.
    memset(&ifr, 0, sizeof(ifr));
    snprintf(ifr.ifr_name, sizeof(ifr.ifr_name), "%s", interface);
    if (ioctl(sd, SIOCGIFHWADDR, &ifr) < 0) {
        perror("ioctl() failed to get source MAC address ");
        exit(EXIT_FAILURE);
    }
    close(sd);

    // Copy source MAC address.
    memcpy(src_mac, ifr.ifr_hwaddr.sa_data, 6 * sizeof(uint8_t));

    // Report source MAC address to stdout.
    std::cout << "MAC address for interface " << interface << " is ";
    printMACAddress(src_mac);

    // Find interface index from interface name and store index in
    // struct sockaddr_ll device, which will be used as an argument of sendto().
    memset(&device, 0, sizeof(device));
    if ((device.sll_ifindex = if_nametoindex(interface)) == 0) {
        perror("if_nametoindex() failed to obtain interface index ");
        exit(EXIT_FAILURE);
    }
    std::cout << "Index for interface " << interface << " is " << device.sll_ifindex << '\n';

    // Set destination MAC address: you need to fill these out
    dst_mac[0] = 0xff;
    dst_mac[1] = 0xff;
    dst_mac[2] = 0xff;
    dst_mac[3] = 0xff;
    dst_mac[4] = 0xff;
    dst_mac[5] = 0xff;

    // Source IPv4 address: you need to fill this out
    strcpy(src_ip, "192.168.0.240");

    // Destination URL or IPv4 address: you need to fill this out
    strcpy(target, "www.google.com");

    // Fill out hints for getaddrinfo().
    memset(&hints, 0, sizeof(struct addrinfo));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = hints.ai_flags | AI_CANONNAME;

    // Resolve target using getaddrinfo().
    if ((status = getaddrinfo(target, NULL, &hints, &res)) != 0) {
        std::cerr << "getaddrinfo() failed for target: " << gai_strerror(status) << '\n';
        exit(EXIT_FAILURE);
    }
    ipv4 = (struct sockaddr_in *)res->ai_addr;
    tmp = &(ipv4->sin_addr);
    if (inet_ntop(AF_INET, tmp, dst_ip, INET_ADDRSTRLEN) == NULL) {
        status = errno;
        std::cerr << "inet_ntop() failed for target.\nError message: " << strerror(status) << '\n';
        exit(EXIT_FAILURE);
    }
    freeaddrinfo(res);

    // Fill out sockaddr_ll.
    device.sll_family = AF_PACKET;
    memcpy(device.sll_addr, src_mac, 6 * sizeof(uint8_t));
    device.sll_halen = 6;

    // UDP data
    datalen = 4;
    data[0] = 'T';
    data[1] = 'e';
    data[2] = 's';
    data[3] = 't';

    // IPv4 header

    // IPv4 header length (4 bits): Number of 32-bit words in header = 5
    iphdr.ip_hl = IP4_HDRLEN / sizeof(uint32_t);

    // Internet Protocol version (4 bits): IPv4
    iphdr.ip_v = 4;

    // Type of service (8 bits)
    iphdr.ip_tos = 0;

    // Total length of datagram (16 bits): IP header + UDP header + datalen
    iphdr.ip_len = htons(IP4_HDRLEN + UDP_HDRLEN + datalen);

    // ID sequence number (16 bits): unused, since single datagram
    iphdr.ip_id = htons(0);

    // Flags, and Fragmentation offset (3, 13 bits): 0 since single datagram

    // Zero (1 bit)
    ip_flags[0] = 0;

    // Do not fragment flag (1 bit)
    ip_flags[1] = 0;

    // More fragments following flag (1 bit)
    ip_flags[2] = 0;

    // Fragmentation offset (13 bits)
    ip_flags[3] = 0;

    iphdr.ip_off = htons((ip_flags[0] << 15) + (ip_flags[1] << 14) + (ip_flags[2] << 13) + ip_flags[3]);

    // Time-to-Live (8 bits): default to maximum value
    iphdr.ip_ttl = 255;

    // Transport layer protocol (8 bits): 17 for UDP
    iphdr.ip_p = IPPROTO_UDP;

    // Source IPv4 address (32 bits)
    if ((status = inet_pton(AF_INET, src_ip, &(iphdr.ip_src))) != 1) {
        std::cerr << "inet_pton() failed for source address.\nError message: " << strerror(status) << '\n';
        exit(EXIT_FAILURE);
    }

    // Destination IPv4 address (32 bits)
    if ((status = inet_pton(AF_INET, dst_ip, &(iphdr.ip_dst))) != 1) {
        std::cerr << "inet_pton() failed for destination address.\nError message: " << strerror(status) << '\n';
        exit(EXIT_FAILURE);
    }

    // IPv4 header checksum (16 bits): set to 0 when calculating checksum
    iphdr.ip_sum = 0;
    iphdr.ip_sum = checksum((uint16_t *)&iphdr, IP4_HDRLEN);

    // UDP header

    // Source port number (16 bits): pick a number
    udphdr.source = htons(4950);

    // Destination port number (16 bits): pick a number
    udphdr.dest = htons(4950);

    // Length of UDP datagram (16 bits): UDP header + UDP data
    udphdr.len = htons(UDP_HDRLEN + datalen);

    // UDP checksum (16 bits)
    udphdr.check = udp4_checksum(iphdr, udphdr, data, datalen);

    // Fill out ethernet frame header.

    // Ethernet frame length = ethernet header (MAC + MAC + ethernet type) + ethernet data (IP header + UDP header + UDP data)
    frame_length = 6 + 6 + 2 + IP4_HDRLEN + UDP_HDRLEN + datalen;

    // Destination and Source MAC addresses
    memcpy(ether_frame, dst_mac, 6 * sizeof(uint8_t));
    memcpy(ether_frame + 6, src_mac, 6 * sizeof(uint8_t));

    // Next is ethernet type code (ETH_P_IP for IPv4).
    // http://www.iana.org/assignments/ethernet-numbers
    ether_frame[12] = ETH_P_IP / 256;
    ether_frame[13] = ETH_P_IP % 256;

    // Next is ethernet frame data (IPv4 header + UDP header + UDP data).

    // IPv4 header
    memcpy(ether_frame + ETH_HDRLEN, &iphdr, IP4_HDRLEN * sizeof(uint8_t));

    // UDP header
    memcpy(ether_frame + ETH_HDRLEN + IP4_HDRLEN, &udphdr, UDP_HDRLEN * sizeof(uint8_t));

    // UDP data
    memcpy(ether_frame + ETH_HDRLEN + IP4_HDRLEN + UDP_HDRLEN, data, datalen * sizeof(uint8_t));

    // Submit request for a raw socket descriptor.
    if ((sd = socket(PF_PACKET, SOCK_RAW, htons(ETH_P_ALL))) < 0) {
        perror("socket() failed ");
        exit(EXIT_FAILURE);
    }

    // Send ethernet frame to socket.
    if ((bytes = sendto(sd, ether_frame, frame_length, 0, (struct sockaddr *)&device, sizeof(device))) <= 0) {
        perror("sendto() failed");
        exit(EXIT_FAILURE);
    }

    // Close socket descriptor.
    close(sd);

    // Free allocated memory.
    free(src_mac);
    free(dst_mac);
    free(data);
    free(ether_frame);
    free(interface);
    free(target);
    free(src_ip);
    free(dst_ip);
    free(ip_flags);

    return EXIT_SUCCESS;
}

// Computing the internet checksum (RFC 1071).
// Note that the internet checksum is not guaranteed to preclude collisions.
uint16_t checksum(uint16_t *addr, int len) {
    int count = len;
    register uint32_t sum = 0;
    uint16_t answer = 0;

    // Sum up 2-byte values until none or only one byte left.
    while (count > 1) {
        sum += *(addr++);
        count -= 2;
    }

    // Add left-over byte, if any.
    if (count > 0) {
        sum += *(uint8_t *)addr;
    }

    // Fold 32-bit sum into 16 bits; we lose information by doing this,
    // increasing the chances of a collision.
    // sum = (lower 16 bits) + (upper 16 bits shifted right 16 bits)
    while (sum >> 16) {
        sum = (sum & 0xffff) + (sum >> 16);
    }

    // Checksum is one's compliment of sum.
    answer = ~sum;

    return answer;
}

// Build IPv4 UDP pseudo-header and call checksum function.
uint16_t udp4_checksum(struct ip iphdr, struct udphdr udphdr, uint8_t *payload, int payloadlen) {
    char buf[IP_MAXPACKET];
    char *ptr;
    int chksumlen = 0;
    int i;

    ptr = &buf[0];  // ptr points to beginning of buffer buf

    // Copy source IP address into buf (32 bits)
    memcpy(ptr, &iphdr.ip_src.s_addr, sizeof(iphdr.ip_src.s_addr));
    ptr += sizeof(iphdr.ip_src.s_addr);
    chksumlen += sizeof(iphdr.ip_src.s_addr);

    // Copy destination IP address into buf (32 bits)
    memcpy(ptr, &iphdr.ip_dst.s_addr, sizeof(iphdr.ip_dst.s_addr));
    ptr += sizeof(iphdr.ip_dst.s_addr);
    chksumlen += sizeof(iphdr.ip_dst.s_addr);

    // Copy zero field to buf (8 bits)
    *ptr = 0;
    ptr++;
    chksumlen += 1;

    // Copy transport layer protocol to buf (8 bits)
    memcpy(ptr, &iphdr.ip_p, sizeof(iphdr.ip_p));
    ptr += sizeof(iphdr.ip_p);
    chksumlen += sizeof(iphdr.ip_p);

    // Copy UDP length to buf (16 bits)
    memcpy(ptr, &udphdr.len, sizeof(udphdr.len));
    ptr += sizeof(udphdr.len);
    chksumlen += sizeof(udphdr.len);

    // Copy UDP source port to buf (16 bits)
    memcpy(ptr, &udphdr.source, sizeof(udphdr.source));
    ptr += sizeof(udphdr.source);
    chksumlen += sizeof(udphdr.source);

    // Copy UDP destination port to buf (16 bits)
    memcpy(ptr, &udphdr.dest, sizeof(udphdr.dest));
    ptr += sizeof(udphdr.dest);
    chksumlen += sizeof(udphdr.dest);

    // Copy UDP length again to buf (16 bits)
    memcpy(ptr, &udphdr.len, sizeof(udphdr.len));
    ptr += sizeof(udphdr.len);
    chksumlen += sizeof(udphdr.len);

    // Copy UDP checksum to buf (16 bits)
    // Zero, since we don't know it yet
    *ptr = 0;
    ptr++;
    *ptr = 0;
    ptr++;
    chksumlen += 2;

    // Copy payload to buf
    memcpy(ptr, payload, payloadlen);
    ptr += payloadlen;
    chksumlen += payloadlen;

    // Pad to the next 16-bit boundary
    for (i = 0; i < payloadlen % 2; i++, ptr++) {
        *ptr = 0;
        ptr++;
        chksumlen++;
    }

    return checksum((uint16_t *)buf, chksumlen);
}

// Allocate memory for an array of chars.
char *allocate_strmem(int len) {
    void *tmp;

    if (len <= 0) {
        std::cerr << "ERROR: Cannot allocate memory because len = " << len << " in allocate_strmem().\n";
        exit(EXIT_FAILURE);
    }

    tmp = (char *)malloc(len * sizeof(char));
    if (tmp != NULL) {
        memset(tmp, 0, len * sizeof(char));
        return (char *)tmp;
    } else {
        std::cerr << "ERROR: Cannot allocate memory for array allocate_strmem().\n";
        exit(EXIT_FAILURE);
    }
}

// Allocate memory for an array of unsigned chars.
uint8_t *allocate_ustrmem(int len) {
    void *tmp;

    if (len <= 0) {
        std::cerr << "ERROR: Cannot allocate memory because len = " << len << " in allocate_ustrmem().\n";
        exit(EXIT_FAILURE);
    }

    tmp = (uint8_t *)malloc(len * sizeof(uint8_t));
    if (tmp != NULL) {
        memset(tmp, 0, len * sizeof(uint8_t));
        return (uint8_t *)tmp;
    } else {
        std::cerr << "ERROR: Cannot allocate memory for array allocate_ustrmem().\n";
        exit(EXIT_FAILURE);
    }
}

// Allocate memory for an array of ints.
int *allocate_intmem(int len) {
    void *tmp;

    if (len <= 0) {
        std::cerr << "ERROR: Cannot allocate memory because len = " << len << " in allocate_intmem().\n";
        exit(EXIT_FAILURE);
    }

    tmp = (int *)malloc(len * sizeof(int));
    if (tmp != NULL) {
        memset(tmp, 0, len * sizeof(int));
        return (int *)tmp;
    } else {
        std::cerr << "ERROR: Cannot allocate memory for array allocate_intmem().\n";
        exit(EXIT_FAILURE);
    }
}
