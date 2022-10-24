#include <arpa/inet.h>
#include <errno.h>
#include <netdb.h>
#include <netinet/in.h>
#include <signal.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

#include <cstring>
#include <iostream>
using namespace std;

#define PORT 4950  // The port clients will be connecting to

#define BACKLOG 10  // How many pending connections queue will hold

// UDPServer class to create a UDP server
class UDPServer {
    int sockfd;                                // Socket file descriptor on which the server will listen
    int port;                                  // Port on which the server will be bound to
    string myIP;                               // IP address of the server
    struct addrinfo *serverAddr, *serverInfo;  // Server's address information
    const int MAX_BUFF_LEN;                    // Maximum buffer length while receiving the data

   public:
    // Constructor
    UDPServer(int port, int &status, int domain = AF_UNSPEC /* Don't care IPv4 or IPv6 */, int maxBufLen = 1e6)
        : sockfd{-1}, port{port}, myIP{""}, serverAddr{nullptr}, serverInfo{nullptr}, MAX_BUFF_LEN{maxBufLen} {
        // Mark the status as -1 to denote error
        status = -1;

        // The structs required for providing hints
        struct addrinfo hints;

        // Zero out the struct for providing hints
        memset(&hints, 0, sizeof(hints));

        hints.ai_family = domain;        // AF_INET or AF_INET6
        hints.ai_socktype = SOCK_DGRAM;  // For UDP socket
        hints.ai_flags = AI_PASSIVE;     // Use my IP

        int rv = 0;  // Return value

        // Get the server address info
        if ((rv = getaddrinfo(nullptr, to_string(port).c_str(), &hints, &serverInfo)) != 0) {
            std::cerr << "server: getaddrinfo: " << gai_strerror(rv) << "\n";
            return;
        }

        // loop through all the results and bind to the first we can
        for (serverAddr = serverInfo; serverAddr != nullptr; serverAddr = serverAddr->ai_next) {
            // Create a socket with the same domain, type and protocol as used by the server
            if ((sockfd = socket(serverAddr->ai_family, serverAddr->ai_socktype, serverAddr->ai_protocol)) == -1) {
                perror("server: socket");
                continue;
            }

            // Allow the port to be used again
            int yes = 1;
            if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int)) == -1) {
                perror("server: setsockopt");
                return;
            }

            // Bind the socket
            if (bind(sockfd, serverAddr->ai_addr, serverAddr->ai_addrlen) == -1) {
                // If bind fails, close the socket file descriptor
                close(sockfd);
                perror("server: bind");
                continue;
            }

            // Break as we make and bind the first socket from the results
            break;
        }

        // If while iterating on linked list, we reach the end, then we are unable to create a socket
        if (serverAddr == nullptr) {
            std::cerr << "server: failed to bind\n";
            return;
        }

        // Set my IP address
        myIP = getIP(serverAddr->ai_addr);

        // Mark the return status as 0 to denote success
        status = 0;

        std::cout << "server: successfully created a UDP socket at " << myIP << ":" << port << "\n";
    }

    // Destructor
    ~UDPServer() {
        // Close the socket file descriptor
        if (sockfd != -1) close(sockfd);

        // Release the resources for the server info and the internal linked lis
        if (serverInfo != nullptr) freeaddrinfo(serverInfo);
    }

    // Function to get the socket file descriptor of the server
    int getSockFD() {
        return sockfd;
    }

    // Function to get the port on which the server is listening
    int getPort() {
        return port;
    }

    // Function to get the address information of the server
    addrinfo getServerAddrInfo() {
        return *serverAddr;
    }

    // Function to send data to the client
    int sendData(string data, struct sockaddr_storage *clientAddr, int flags = 0) {
        if (sockfd == -1) return -1;

        // Length of the struct
        socklen_t addrLen = sizeof(*clientAddr);

        // Send data to the client
        if (sendto(sockfd, data.c_str(), data.length(), flags, (struct sockaddr *)clientAddr, addrLen) == -1) {
            perror("server: sendto");
            return -1;
        }

        return 0;
    }

    // Function to receive data from the client
    pair<string, struct sockaddr_storage *> receiveData(int flags = 0) {
        if (sockfd == -1) throw "Invalid socket";

        // The number of bytes read from the client
        int numBytes = 0;

        struct sockaddr_storage *clientAddr = new struct sockaddr_storage();
        socklen_t addrLen = sizeof(*clientAddr);

        // Buffer to read the input message received from the client
        char buff[MAX_BUFF_LEN];
        if ((numBytes = recvfrom(sockfd, buff, MAX_BUFF_LEN - 1, flags, (struct sockaddr *)clientAddr, &addrLen)) == -1) {
            perror("server: recvfrom");
            throw "Unable to receive data";
        }

        // Mark the end of the input message with NULL character to indicate end of string
        buff[numBytes] = '\0';

        return make_pair(string(buff), clientAddr);
    }

    // Function to get `sockaddr_in` if IPv4 or `sockaddr_in6` if IPv6:
    static void *get_in_addr(struct sockaddr *addr) {
        if (addr->sa_family == AF_INET) {
            // If it is an IPv4 address
            return &(((struct sockaddr_in *)addr)->sin_addr);
        }

        // If it is an IPv6 address
        return &(((struct sockaddr_in6 *)addr)->sin6_addr);
    }

    // Function to return the IP address from an address information
    static string getIP(struct sockaddr *addr) {
        if (addr->sa_family == AF_INET) {
            // If it is an IPv4 address
            // Buffer to populate the IP address
            char ipv4[INET_ADDRSTRLEN];
            // Convert the network address into the IP address
            inet_ntop(addr->sa_family, get_in_addr(addr), ipv4, INET_ADDRSTRLEN);
            return string(ipv4);
        }

        // If it is an IPv6 address
        // Buffer to populate the IP address
        char ipv6[INET6_ADDRSTRLEN];
        // Convert the network address into the IP address
        inet_ntop(addr->sa_family, get_in_addr(addr), ipv6, INET6_ADDRSTRLEN);
        return string(ipv6);
    }

    // Function to return the Port from an address information
    static in_port_t getPort(struct sockaddr *addr) {
        if (addr->sa_family == AF_INET) {
            // If it is an IPv4 address
            return ((struct sockaddr_in *)get_in_addr(addr))->sin_port;
        }

        // If it is an IPv6 address
        return ((struct sockaddr_in6 *)get_in_addr(addr))->sin6_port;
    }
};

// Handler for SIGCHLD exception
void sigchld_handler(int s) {
    // waitpid() might overwrite errno, so we save and restore it:
    int saved_errno = errno;

    while (waitpid(-1, NULL, WNOHANG) > 0)
        ;

    errno = saved_errno;
}

int main() {
    int status = 0;
    // Creating a UDP server
    UDPServer server(PORT, status, AF_INET);
    if (status == -1) {
        exit(1);
    }

    struct sigaction sa;
    sa.sa_handler = sigchld_handler;  // reap all dead processes
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = SA_RESTART;
    if (sigaction(SIGCHLD, &sa, NULL) == -1) {
        perror("sigaction");
        exit(1);
    }

    while (1) {
        try {
            auto p = server.receiveData();

            std::cout << "server: got packet from " << UDPServer::getIP((struct sockaddr *)p.second) << ":" << UDPServer::getPort((struct sockaddr *)p.second) << "\n";

            cout << "server: packet contains \"" << p.first << "\"\n";

            if (server.sendData("Hi from server!", p.second) == -1) {
                exit(1);
            }
        } catch (exception &ex) {
            exit(1);
        }
    }

    return 0;
}
