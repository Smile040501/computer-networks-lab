// 111901030
// Mayank Singla

#include <arpa/inet.h>         // inet_pton() and inet_ntop()
#include <errno.h>             // errno, perror()
#include <linux/if_packet.h>   // struct sockaddr_ll (see man 7 packet)
#include <net/ethernet.h>      // struct ethhdr
#include <netinet/if_ether.h>  // For ETH_P_IP
#include <netinet/in.h>        // IPPROTO_RAW, IPPROTO_IP, IPPROTO_TCP, IPPROTO_UDP, INET_ADDRSTRLEN
#include <netinet/ip.h>        // struct ip and IP_MAXPACKET (which is 65535)
#include <sys/socket.h>        // needed for socket()
#include <sys/types.h>         // needed for socket(), uint8_t, uint16_t, uint32_t
#include <unistd.h>            // close()

#include <iomanip>
#include <iostream>

using namespace std;
using ll = long long;

// Function to print the MAC address in a pretty format
void printMACAddress(unsigned char *addr) {
    std::ios init(NULL);

    // copying the default `cout` formatting
    init.copyfmt(std::cout);

    // Converting each character to unsigned int and printing it in hex form
    for (ll i = 0; i < ETH_ALEN; ++i) {
        std::cout << std::setfill('0') << std::setw(2) << std::hex << static_cast<unsigned int>(addr[i]);
        if (i != ETH_ALEN - 1) std::cout << ':';
    }

    // resetting the default `cout` formatting
    std::cout.copyfmt(init);
}

// Function to print the source and destination MAC addresses from the packet buffer
void printEthernetHeader(unsigned char *buffer) {
    // Populate the fields of the ethhdr struct
    struct ethhdr *eth = reinterpret_cast<struct ethhdr *>(buffer);

    // Extract the source MAC address
    std::cout << std::setw(25) << std::left << "Source MAC Address"
              << " : ";
    printMACAddress(eth->h_source);
    std::cout << '\n';

    // Extract the destination MAC address
    std::cout << std::setw(25) << std::left << "Destination MAC Address"
              << " : ";
    printMACAddress(eth->h_dest);
    std::cout << '\n';
}

// Function to print the source and destination IP addresses from the packet buffer
void printIpHeader(unsigned char *buffer) {
    // Populate IPv4 header by skipping the size of the struct ethhdr
    struct ip *iphdr = reinterpret_cast<struct ip *>(buffer + sizeof(struct ethhdr));

    // space to hold the IPv4 string
    char src_ip[INET_ADDRSTRLEN], dest_ip[INET_ADDRSTRLEN];

    // Extract the source and destination IP addresses
    inet_ntop(AF_INET, &(iphdr->ip_src), src_ip, INET_ADDRSTRLEN);
    inet_ntop(AF_INET, &(iphdr->ip_dst), dest_ip, INET_ADDRSTRLEN);

    std::cout << std::setw(25) << std::left << "Source IP Address"
              << " : " << src_ip << '\n';
    std::cout << std::setw(25) << std::left << "Destination IP Address"
              << " : " << dest_ip << '\n';
}

int main() {
    std::cout << "Sniffing...\n";

    // Submit request for a raw socket descriptor to listen for all the IP packets
    ll sd = 0;
    if ((sd = socket(AF_PACKET, SOCK_RAW, htons(ETH_P_IP))) < 0) {
        perror("socket");
        exit(EXIT_FAILURE);
    }

    // Create the packet buffer to receive the incoming packet
    unsigned char *buffer = new unsigned char[IP_MAXPACKET + 1];

    // The client address field
    struct sockaddr_ll saddr;
    ll saddrLen = 0;

    // Keep listening for the incoming packets in a recvfrom loop
    while (true) {
        // The size of the client address field
        saddrLen = sizeof(saddr);

        // Receiving a packet
        ll numBytes = 0;
        if ((numBytes = recvfrom(sd, buffer, IP_MAXPACKET + 1, 0, reinterpret_cast<struct sockaddr *>(&saddr), reinterpret_cast<socklen_t *>(&saddrLen))) < 0) {
            perror("recvfrom");
            exit(EXIT_FAILURE);
        }

        // Print source and destination MAC addresses
        printEthernetHeader(buffer);

        // Print source and destination IP addresses
        printIpHeader(buffer);

        std::cout << '\n';
    }

    // Free up the resources
    delete[] buffer;

    // Close the socket file descriptor
    close(sd);

    return EXIT_SUCCESS;
}
