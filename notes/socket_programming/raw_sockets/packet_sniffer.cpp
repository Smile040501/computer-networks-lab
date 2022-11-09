#include <arpa/inet.h>         // inet_pton() and inet_ntop()
#include <errno.h>             // errno, perror()
#include <net/ethernet.h>      // struct ethhdr
#include <netdb.h>             // struct addrinfo
#include <netinet/if_ether.h>  // For ETH_P_ALL
#include <netinet/in.h>        // IPPROTO_RAW, IPPROTO_IP, IPPROTO_TCP, INET_ADDRSTRLEN
#include <netinet/ip.h>        // struct ip and IP_MAXPACKET (which is 65535)
#include <netinet/ip_icmp.h>   // struct icmphdr
#include <netinet/tcp.h>       // struct tcphdr
#include <netinet/udp.h>       // struct udphdr
#include <sys/ioctl.h>         // macro ioctl is defined
#include <sys/socket.h>        // needed for socket()
#include <sys/types.h>         // needed for socket(), uint8_t, uint16_t, uint32_t
#include <unistd.h>            // close()

#include <cstring>
#include <fstream>
#include <iomanip>
#include <iostream>
using namespace std;

void processPacket(unsigned char *, int);
void printIPheader(unsigned char *, int);
void printTCPpacket(unsigned char *, int);
void printUDPpacket(unsigned char *, int);
void printICMPpacket(unsigned char *, int);
void printData(unsigned char *, int);

std::ofstream logfile("log.txt");
int tcp = 0, udp = 0, icmp = 0, others = 0, igmp = 0, total = 0, i, j;

int main() {
    int saddr_size, data_size;
    struct sockaddr saddr;

    unsigned char *buffer = (unsigned char *)malloc(IP_MAXPACKET + 1);  // Its Big!

    if (!logfile) {
        std::cerr << "Unable to create log.txt file.\n";
    }
    std::cout << "Starting...\n";

    int sock_raw = 0;
    // setsockopt(sock_raw , SOL_SOCKET , SO_BINDTODEVICE , "eth0" , strlen("eth0")+ 1 );

    if ((sock_raw = socket(AF_PACKET, SOCK_RAW, htons(ETH_P_ALL))) < 0) {
        // Print the error with proper message
        perror("Socket Error");
        exit(EXIT_FAILURE);
    }

    while (true) {
        saddr_size = sizeof saddr;
        // Receive a packet
        data_size = recvfrom(sock_raw, buffer, IP_MAXPACKET + 1, 0, &saddr, (socklen_t *)&saddr_size);
        if (data_size < 0) {
            std::cerr << "Recvfrom error , failed to get packets\n";
            exit(EXIT_FAILURE);
        }
        // Now process the packet
        processPacket(buffer, data_size);
    }
    logfile.close();
    close(sock_raw);
    std::cout << "Finished\n";
    return EXIT_SUCCESS;
}

void processPacket(unsigned char *buffer, int size) {
    // Get the IP Header part of this packet , excluding the ethernet header
    struct iphdr *iph = (struct iphdr *)(buffer + sizeof(struct ethhdr));
    ++total;
    switch (iph->protocol)  // Check the Protocol and do accordingly...
    {
        case 1:  // ICMP Protocol
            ++icmp;
            printICMPpacket(buffer, size);
            break;

        case 2:  // IGMP Protocol
            ++igmp;
            break;

        case 6:  // TCP Protocol
            ++tcp;
            printTCPpacket(buffer, size);
            break;

        case 17:  // UDP Protocol
            ++udp;
            printUDPpacket(buffer, size);
            break;

        default:  // Some Other Protocol like ARP etc.
            ++others;
            break;
    }

    std::cout << "TCP : " << tcp << "   UDP : " << udp << "   ICMP : " << icmp << "   IGMP : " << igmp << "   Others : " << others << "   Total : " << total << '\r';
}

void printMACAddress(unsigned char *addr) {
    std::ios init(NULL);
    // copying the default `cout` formatting
    init.copyfmt(std::cout);
    for (int i = 0; i < ETH_ALEN; ++i) {
        logfile << std::setfill('0') << std::setw(2) << std::hex << static_cast<unsigned int>(addr[i]);
        if (i != ETH_ALEN - 1) logfile << ':';
    }
    // resetting the default `cout` formatting
    std::cout.copyfmt(init);
}

void printEthernetHeader(unsigned char *buffer, int size) {
    struct ethhdr *eth = (struct ethhdr *)buffer;

    logfile << "\nEthernet Header\n";
    logfile << "   |-Destination Address : ";
    printMACAddress(eth->h_dest);
    logfile << '\n';
    logfile << "   |-Source Address      : ";
    printMACAddress(eth->h_source);
    logfile << '\n';
    logfile << "   |-Protocol            : ";
    logfile << (unsigned short)eth->h_proto;
}

void printIPheader(unsigned char *buffer, int size) {
    printEthernetHeader(buffer, size);

    unsigned short iphdrlen;

    struct ip *iph = (struct ip *)(buffer + sizeof(struct ethhdr));
    iphdrlen = iph->ip_hl * 4;

    char src_ip[INET_ADDRSTRLEN];  // space to hold the IPv4 string
    inet_ntop(AF_INET, &(iph->ip_src), src_ip, INET_ADDRSTRLEN);

    char dest_ip[INET_ADDRSTRLEN];  // space to hold the IPv4 string
    inet_ntop(AF_INET, &(iph->ip_dst), dest_ip, INET_ADDRSTRLEN);

    logfile << "\nIP Header\n";
    logfile << "   |-IP Version        : " << (unsigned int)iph->ip_v << '\n';
    logfile << "   |-IP Header Length  : " << (unsigned int)iph->ip_hl << " DWORDS or " << iphdrlen << " Bytes\n";
    logfile << "   |-Type Of Service   : " << (unsigned int)iph->ip_tos << '\n';
    logfile << "   |-IP Total Length   : " << ntohs(iph->ip_len) << "  Bytes(size of Packet)\n";
    logfile << "   |-Identification    : " << ntohs(iph->ip_id) << '\n';
    logfile << "   |-TTL      : " << (unsigned int)iph->ip_ttl << '\n';
    logfile << "   |-Protocol : " << (unsigned int)iph->ip_p << '\n';
    logfile << "   |-Checksum : " << ntohs(iph->ip_sum) << '\n';
    logfile << "   |-Source IP        : " << src_ip << '\n';
    logfile << "   |-Destination IP   : " << dest_ip << '\n';
}

void printTCPpacket(unsigned char *buffer, int size) {
    unsigned short iphdrlen;

    struct ip *iph = (struct ip *)(buffer + sizeof(struct ethhdr));
    iphdrlen = iph->ip_hl * 4;

    struct tcphdr *tcph = (struct tcphdr *)(buffer + iphdrlen + sizeof(struct ethhdr));

    int header_size = sizeof(struct ethhdr) + iphdrlen + tcph->doff * 4;

    logfile << "\n\n***********************TCP Packet*************************\n";

    printIPheader(buffer, size);

    logfile << "\nTCP Header\n";
    logfile << "   |-Source Port      : " << ntohs(tcph->source) << '\n';
    logfile << "   |-Destination Port : " << ntohs(tcph->dest) << '\n';
    logfile << "   |-Sequence Number    : " << ntohl(tcph->seq) << '\n';
    logfile << "   |-Acknowledge Number : " << ntohl(tcph->ack_seq) << '\n';
    logfile << "   |-Header Length      : " << (unsigned int)tcph->doff << " DWORDS or " << (unsigned int)tcph->doff * 4 << " BYTES\n";
    logfile << "   |-Urgent Flag          : " << (unsigned int)tcph->urg << '\n';
    logfile << "   |-Acknowledgement Flag : " << (unsigned int)tcph->ack << '\n';
    logfile << "   |-Push Flag            : " << (unsigned int)tcph->psh << '\n';
    logfile << "   |-Reset Flag           : " << (unsigned int)tcph->rst << '\n';
    logfile << "   |-Synchronise Flag     : " << (unsigned int)tcph->syn << '\n';
    logfile << "   |-Finish Flag          : " << (unsigned int)tcph->fin << '\n';
    logfile << "   |-Window         : " << ntohs(tcph->window) << '\n';
    logfile << "   |-Checksum       : " << ntohs(tcph->check) << '\n';
    logfile << "   |-Urgent Pointer : " << tcph->urg_ptr << '\n';
    logfile << "\n";
    logfile << "                        DATA Dump                         ";
    logfile << "\n";

    logfile << "IP Header\n";
    printData(buffer, iphdrlen);

    logfile << "TCP Header\n";
    printData(buffer + iphdrlen, tcph->doff * 4);

    logfile << "Data Payload\n";
    printData(buffer + header_size, size - header_size);

    logfile << "\n###########################################################";
}

void printUDPpacket(unsigned char *buffer, int size) {
    unsigned short iphdrlen;

    struct ip *iph = (struct ip *)(buffer + sizeof(struct ethhdr));
    iphdrlen = iph->ip_hl * 4;

    struct udphdr *udph = (struct udphdr *)(buffer + iphdrlen + sizeof(struct ethhdr));

    int header_size = sizeof(struct ethhdr) + iphdrlen + sizeof udph;

    logfile << "\n\n***********************UDP Packet*************************\n";

    printIPheader(buffer, size);

    logfile << "\nUDP Header\n";
    logfile << "   |-Source Port      : " << ntohs(udph->source) << '\n';
    logfile << "   |-Destination Port : " << ntohs(udph->dest) << '\n';
    logfile << "   |-UDP Length       : " << ntohs(udph->len) << '\n';
    logfile << "   |-UDP Checksum     : " << ntohs(udph->check) << '\n';

    logfile << "\nIP Header\n";
    printData(buffer, iphdrlen);

    logfile << "UDP Header\n";
    printData(buffer + iphdrlen, sizeof udph);

    logfile << "Data Payload\n";

    // Move the pointer ahead and reduce the size of string
    printData(buffer + header_size, size - header_size);

    logfile << "\n###########################################################";
}

void printICMPpacket(unsigned char *buffer, int size) {
    unsigned short iphdrlen;

    struct ip *iph = (struct ip *)(buffer + sizeof(struct ethhdr));
    iphdrlen = iph->ip_hl * 4;

    struct icmphdr *icmph = (struct icmphdr *)(buffer + iphdrlen + sizeof(struct ethhdr));

    int header_size = sizeof(struct ethhdr) + iphdrlen + sizeof icmph;

    logfile << "\n\n***********************ICMP Packet*************************\n";

    printIPheader(buffer, size);

    logfile << "\nICMP Header\n";
    logfile << "   |-Type : " << (unsigned int)(icmph->type);

    if ((unsigned int)(icmph->type) == 11) {
        logfile << "  (TTL Expired)\n";
    } else if ((unsigned int)(icmph->type) == ICMP_ECHOREPLY) {
        logfile << "  (ICMP Echo Reply)\n";
    }

    logfile << "   |-Code : " << (unsigned int)(icmph->code) << '\n';
    logfile << "   |-Checksum : " << ntohs(icmph->checksum) << '\n';

    logfile << "\nIP Header\n";
    printData(buffer, iphdrlen);

    logfile << "UDP Header\n";
    printData(buffer + iphdrlen, sizeof icmph);

    logfile << "Data Payload\n";

    // Move the pointer ahead and reduce the size of string
    printData(buffer + header_size, (size - header_size));

    logfile << "\n###########################################################";
}

void printData(unsigned char *data, int size) {
    int i, j;
    for (i = 0; i < size; i++) {
        if (i != 0 && i % 16 == 0)  // if one line of hex printing is complete...
        {
            logfile << "         ";
            for (j = i - 16; j < i; j++) {
                if (data[j] >= 32 && data[j] <= 128)
                    logfile << (unsigned char)data[j];  // if its a number or alphabet

                else
                    logfile << ".";  // otherwise print a dot
            }
            logfile << "\n";
        }

        logfile << "   ";
        logfile << (unsigned int)data[i];

        if (i == size - 1)  // print the last spaces
        {
            for (j = 0; j < 15 - i % 16; j++) {
                logfile << "   ";  // extra spaces
            }

            logfile << "         ";

            for (j = i - i % 16; j <= i; j++) {
                if (data[j] >= 32 && data[j] <= 128) {
                    logfile << (unsigned char)data[j];
                } else {
                    logfile << ".";
                }
            }

            logfile << "\n";
        }
    }
}
