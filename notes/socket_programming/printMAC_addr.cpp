// Get MAC Address from Interface Name

#include <net/if.h>            // ifreq
#include <netinet/if_ether.h>  // For ETH_ALEN
#include <sys/ioctl.h>         // macro ioctl is defined
#include <sys/socket.h>        // needed for socket()
#include <unistd.h>            // close()

#include <cstring>
#include <iomanip>
#include <iostream>
using namespace std;

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
}

int main() {
    string iface = "eth0";

    int fd = socket(AF_INET, SOCK_DGRAM, 0);

    struct ifreq ifr;
    ifr.ifr_addr.sa_family = AF_INET;
    strncpy(ifr.ifr_name, iface.c_str(), IFNAMSIZ - 1);

    if (ioctl(fd, SIOCGIFHWADDR, &ifr) == -1) {
        std::cerr << "ioctl\n";
        exit(EXIT_FAILURE);
    }
    // SIOCGIFADDR : For IP address

    close(fd);

    unsigned char *mac = reinterpret_cast<unsigned char *>(ifr.ifr_hwaddr.sa_data);

    // display mac address
    std::cout << "MAC: ";
    printMACAddress(mac);

    return EXIT_SUCCESS;
}
