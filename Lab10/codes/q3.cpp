// 111901030
// Mayank Singla

#include <arpa/inet.h>         // inet_pton() and inet_ntop()
#include <errno.h>             // errno, perror()
#include <linux/if_packet.h>   // struct sockaddr_ll (see man 7 packet)
#include <net/ethernet.h>      // struct ethhdr
#include <net/if.h>            // if_nametoindex()
#include <netinet/if_ether.h>  // For ETH_P_IP
#include <netinet/in.h>        // IPPROTO_RAW, IPPROTO_IP, IPPROTO_TCP, IPPROTO_UDP, INET_ADDRSTRLEN
#include <netinet/ip.h>        // struct ip and IP_MAXPACKET (which is 65535)
#include <sys/ioctl.h>         // macro ioctl is defined
#include <sys/socket.h>        // needed for socket()
#include <sys/types.h>         // needed for socket(), uint8_t, uint16_t, uint32_t
#include <unistd.h>            // close()

#include <cstring>
#include <iomanip>
#include <iostream>

using namespace std;
using ll = long long;

// Interface names to bind to
#define IF_NAME1 "eth1"
#define IF_NAME2 "eth2"

// Function to get the MAC address in a pretty format
string getMACAddress(unsigned char *addr) {
    // Using string stream to store the result
    stringstream ss;

    // Converting each character to unsigned int and printing it in hex form
    for (ll i = 0; i < ETH_ALEN; ++i) {
        ss << std::setfill('0') << std::setw(2) << std::hex << static_cast<unsigned int>(addr[i]);
        if (i != ETH_ALEN - 1) ss << ':';
    }

    return ss.str();
}

// Function to print and return the source and destination MAC addresses from the packet buffer
pair<string, string> printEthernetHeader(unsigned char *buffer) {
    // Populate the fields of the ethhdr struct
    struct ethhdr *eth = reinterpret_cast<struct ethhdr *>(buffer);

    // Extract the source MAC address
    std::cout << std::setw(25) << std::left << "Source MAC Address"
              << " : ";
    string srcMac = getMACAddress(eth->h_source);
    std::cout << srcMac << '\n';

    // Extract the destination MAC address
    std::cout << std::setw(25) << std::left << "Destination MAC Address"
              << " : ";
    string destMac = getMACAddress(eth->h_dest);
    std::cout << destMac << '\n';

    return make_pair(srcMac, destMac);
}

// Function to print and return the source and destination IP addresses from the packet buffer
pair<string, string> printIpHeader(unsigned char *buffer) {
    // Populate IPv4 header by skipping the size of the struct ethhdr
    struct ip *iphdr = reinterpret_cast<struct ip *>(buffer + sizeof(struct ethhdr));

    // space to hold the IPv4 string
    char src_ip[INET_ADDRSTRLEN], dest_ip[INET_ADDRSTRLEN];  // space to hold the IPv4 string

    // Extract the source and destination IP addresses
    inet_ntop(AF_INET, &(iphdr->ip_src), src_ip, INET_ADDRSTRLEN);
    inet_ntop(AF_INET, &(iphdr->ip_dst), dest_ip, INET_ADDRSTRLEN);

    std::cout << std::setw(25) << std::left << "Source IP Address"
              << " : " << src_ip << '\n';
    std::cout << std::setw(25) << std::left << "Destination IP Address"
              << " : " << dest_ip << '\n';

    return make_pair(string(src_ip), string(dest_ip));
}

// Function to create and bind a socket on the specified interface
ll createRecvSocket(const char *ifName) {
    // Submit request for a raw socket descriptor to listen for all the IP packets
    ll sd = 0;
    if ((sd = socket(AF_PACKET, SOCK_RAW, htons(ETH_P_IP))) < 0) {
        perror("socket");
        exit(EXIT_FAILURE);
    }

    // Populate the struct and look up interface index which we will use to bind socket descriptor sd to specified interface
    struct sockaddr_ll device;
    device.sll_family = AF_PACKET;
    device.sll_ifindex = static_cast<int>(if_nametoindex(ifName));
    device.sll_protocol = htons(ETH_P_IP);

    // Bind the socket descriptor to the specified interface
    if (bind(sd, reinterpret_cast<const sockaddr *>(&device), sizeof(device)) < 0) {
        perror("bind");
        close(sd);
        exit(EXIT_FAILURE);
    }

    return sd;
}

// Function to create a raw socket used for sending data
ll createSendSocket() {
    // Submit request for a raw socket descriptor
    ll sd = 0;
    if ((sd = socket(AF_INET, SOCK_RAW, IPPROTO_RAW)) < 0) {
        perror("socket");
        exit(EXIT_FAILURE);
    }

    return sd;
}

// Function to get the MAC and IP address of the given interface
pair<string, string> getIfInfo(const char *ifName) {
    ll sd = 0;
    // Submit request for a socket descriptor to look up interface
    if ((sd = socket(AF_PACKET, SOCK_RAW, htons(ETH_P_ALL))) < 0) {
        perror("socket");
        exit(EXIT_FAILURE);
    }

    // Populate this struct to use with ioctl()
    struct ifreq ifr;
    memset(&ifr, 0, sizeof(ifr));
    ifr.ifr_addr.sa_family = AF_INET;

    // Copy the interface name
    snprintf(ifr.ifr_name, sizeof(ifr.ifr_name), "%s", ifName);

    // Use ioctl() to look up interface name and get the IP address
    if (ioctl(sd, SIOCGIFADDR, &ifr) < 0) {
        perror("ioctl");
        exit(EXIT_FAILURE);
    }

    // Extract the IP address
    char ip4[INET_ADDRSTRLEN];
    inet_ntop(AF_INET, &(reinterpret_cast<struct sockaddr_in *>(&ifr.ifr_addr)->sin_addr), ip4, INET_ADDRSTRLEN);
    string ipStr(ip4);

    // Use ioctl() to look up interface name and get the IP address
    if (ioctl(sd, SIOCGIFHWADDR, &ifr) < 0) {
        perror("ioctl");
        exit(EXIT_FAILURE);
    }

    // Extract the MAC address
    unsigned char *mac = reinterpret_cast<unsigned char *>(ifr.ifr_hwaddr.sa_data);
    string macStr = getMACAddress(mac);

    // Close the socket descriptor
    close(sd);

    return make_pair(ipStr, macStr);
}

// Function to read the data from the interface
ll readDataFromInterface(ll sd, unsigned char *buffer, sockaddr_ll *saddr) {
    // Receiving a packet
    ll saddrLen = sizeof(*saddr);
    ll numBytes = recvfrom(sd, buffer, IP_MAXPACKET + 1, 0, reinterpret_cast<struct sockaddr *>(saddr), reinterpret_cast<socklen_t *>(&saddrLen));

    // If an error occurred
    if (numBytes < 0) {
        perror("recvfrom");
        close(sd);
        exit(EXIT_FAILURE);
    }
    return numBytes;
}

int main() {
    fd_set master;   // master file descriptor list for select()
    fd_set readFds;  // temp file descriptor list for select()
    ll fdMax = -1;   // maximum file descriptor number

    FD_ZERO(&master);   // clear the master sets
    FD_ZERO(&readFds);  // clear the temp sets

    // Create receiving and sending socket on interface eth1
    ll eth1Recv = createRecvSocket(IF_NAME1);
    ll eth1Send = createSendSocket();

    // Update the max socket descriptor value
    fdMax = max(fdMax, eth1Recv);
    fdMax = max(fdMax, eth1Send);

    // Create receiving and sending socket on interface eth2
    ll eth2Recv = createRecvSocket(IF_NAME2);
    ll eth2Send = createSendSocket();

    // Update the max socket descriptor value
    fdMax = max(fdMax, eth2Recv);
    fdMax = max(fdMax, eth2Send);

    // Add the receiving socket descriptors to the master file descriptor list
    FD_SET(eth1Recv, &master);
    FD_SET(eth2Recv, &master);

    // Get the MAC and IP addresses of both the interfaces
    pair<string, string> if1Info = getIfInfo(IF_NAME1);
    pair<string, string> if2Info = getIfInfo(IF_NAME2);

    std::cout << "Router Booted...\n\n";

    for (;;) {
        // Initialize the temp set
        readFds = master;

        // Wait of any file descriptor to become ready to read
        if (select(fdMax + 1, &readFds, nullptr, nullptr, nullptr) <= 0) {
            perror("select");
            exit(EXIT_FAILURE);
        }

        // If a packet arrives on the interface eth1
        if (FD_ISSET(eth1Recv, &readFds)) {
            // The client address field
            struct sockaddr_ll saddr;

            // Create the packet buffer to receive the incoming packet
            unsigned char *buffer = new unsigned char[IP_MAXPACKET + 1];

            // Receiving a packet
            ll numBytes = readDataFromInterface(eth1Recv, buffer, &saddr);

            // Check if the packet is not an outgoing packet
            if (static_cast<int>(saddr.sll_pkttype) != PACKET_OUTGOING) {
                // Print the packet details
                printEthernetHeader(buffer);
                string destIp = printIpHeader(buffer).second;
                std::cout << '\n';

                // Check if the packet is not destined to eth1
                if (destIp != if1Info.first) {
                    // Populate the IP header
                    struct ip *iphdr = reinterpret_cast<struct ip *>(buffer + sizeof(struct ethhdr));

                    // Create the destination address to send the packet
                    struct sockaddr_in sin;
                    memset(&sin, 0, sizeof(struct sockaddr_in));
                    sin.sin_family = AF_INET;
                    sin.sin_addr.s_addr = iphdr->ip_dst.s_addr;

                    socklen_t sinSize = sizeof(sin);

                    // Discarding ethernet header
                    buffer = buffer + sizeof(struct ethhdr);

                    // The remaining number of bytes to send
                    unsigned long remBytes = static_cast<unsigned long>(numBytes) - sizeof(struct ethhdr);

                    // Send the packet
                    if (sendto(eth2Send, buffer, remBytes, 0, reinterpret_cast<struct sockaddr *>(&sin), sinSize) < 0) {
                        perror("sendto");
                        close(eth2Send);
                        exit(EXIT_FAILURE);
                    }
                }
            }
        } else if (FD_ISSET(eth2Recv, &readFds)) {
            // If a packet arrives on the interface eth2

            // The client address field
            struct sockaddr_ll saddr;

            // Create the packet buffer to receive the incoming packet
            unsigned char *buffer = new unsigned char[IP_MAXPACKET + 1];

            // Receiving a packet
            ll numBytes = readDataFromInterface(eth2Recv, buffer, &saddr);

            // Check if the packet is not an outgoing packet
            if (static_cast<int>(saddr.sll_pkttype) != PACKET_OUTGOING) {
                // Print the packet details
                printEthernetHeader(buffer);
                string destIp = printIpHeader(buffer).second;
                std::cout << '\n';

                // Check if the packet is not destined to eth2
                if (destIp != if2Info.first) {
                    // Populate the IP header
                    struct ip *iphdr = reinterpret_cast<struct ip *>(buffer + sizeof(struct ethhdr));

                    // Create the destination address to send the packet
                    struct sockaddr_in sin;
                    memset(&sin, 0, sizeof(struct sockaddr_in));
                    sin.sin_family = AF_INET;
                    sin.sin_addr.s_addr = iphdr->ip_dst.s_addr;

                    socklen_t sinSize = sizeof(sin);

                    // Discarding ethernet header
                    buffer = buffer + sizeof(struct ethhdr);

                    // The remaining number of bytes to send
                    unsigned long remBytes = static_cast<unsigned long>(numBytes) - sizeof(struct ethhdr);

                    // Send the packet
                    if (sendto(eth1Send, buffer, remBytes, 0, reinterpret_cast<struct sockaddr *>(&sin), sinSize) < 0) {
                        perror("sendto");
                        close(eth1Send);
                        exit(EXIT_FAILURE);
                    }
                }
            }
        }
    }

    // Close all the socket descriptors
    close(eth2Send);
    close(eth2Recv);
    close(eth1Send);
    close(eth1Recv);

    return EXIT_SUCCESS;
}
