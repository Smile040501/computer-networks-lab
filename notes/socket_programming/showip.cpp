#include <arpa/inet.h>   // inet_pton() and inet_ntop()
#include <netdb.h>       // struct addrinfo
#include <netinet/in.h>  // IPPROTO_RAW, IPPROTO_IP, IPPROTO_TCP, INET_ADDRSTRLEN
#include <sys/socket.h>  // needed for socket()
#include <sys/types.h>   // needed for socket(), uint8_t, uint16_t, uint32_t

#include <cstring>
#include <iostream>
using namespace std;

int main(int argc, char *argv[]) {
    struct addrinfo hints, *res, *p;
    int status;
    char ipstr[INET6_ADDRSTRLEN];

    if (argc != 2) {
        std::cerr << "usage: ./showip <hostname>\n";
        exit(EXIT_FAILURE);
    }

    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_UNSPEC;  // AF_INET or AF_INET6 to force version
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE;  // use my IP

    if ((status = getaddrinfo(argv[1], NULL, &hints, &res)) != 0) {
        std::cerr << "getaddrinfo: " << gai_strerror(status) << "\n";
        exit(EXIT_FAILURE);
    }

    std::cout << "IP addresses for " << argv[1] << "\n\n";

    for (p = res; p != NULL; p = p->ai_next) {
        void *addr;
        string ipver;

        // get the pointer to the address itself,
        // different fields in IPv4 and IPv6:
        if (p->ai_family == AF_INET) {  // IPv4
            struct sockaddr_in *ipv4 = reinterpret_cast<struct sockaddr_in *>(p->ai_addr);
            addr = &(ipv4->sin_addr);
            ipver = "IPv4";
        } else {  // IPv6
            struct sockaddr_in6 *ipv6 = reinterpret_cast<struct sockaddr_in6 *>(p->ai_addr);
            addr = &(ipv6->sin6_addr);
            ipver = "IPv6";
        }

        // convert the IP to a string and print it:
        inet_ntop(p->ai_family, addr, ipstr, sizeof(ipstr));
        std::cout << ipver << " " << ipstr << endl;
    }

    freeaddrinfo(res);  // free the linked list

    return 0;
}
