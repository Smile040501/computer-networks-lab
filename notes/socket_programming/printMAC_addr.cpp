// Get MAC Address from Interface Name

#include <net/if.h>  // ifreq
#include <string.h>  // strncpy
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <unistd.h>  // close

#include <iostream>
using namespace std;

int main() {
    string iface = "eth0";

    int fd = socket(AF_INET, SOCK_DGRAM, 0);

    struct ifreq ifr;
    ifr.ifr_addr.sa_family = AF_INET;
    strncpy(ifr.ifr_name, iface.c_str(), IFNAMSIZ - 1);

    ioctl(fd, SIOCGIFHWADDR, &ifr);
    // SIOCGIFADDR : For IP address

    close(fd);

    unsigned char *mac = (unsigned char *)ifr.ifr_hwaddr.sa_data;

    // display mac address
    printf("Mac : %.2x:%.2x:%.2x:%.2x:%.2x:%.2x\n", mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);

    return 0;
}
