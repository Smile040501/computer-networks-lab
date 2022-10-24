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

#define PORT 3490  // Port on which the server is listening

// TCPClient class to create a TCP client
class TCPClient {
    int sockfd;                                // Socket file descriptor which the client will use to communicate
    int port;                                  // Port on which the server is listening
    string serverIP;                           // IP address of the server
    struct addrinfo *serverAddr, *serverInfo;  // Server's address information
    const int MAX_BUFF_LEN;                    // Maximum buffer length while receiving the data

   public:
    // Constructor
    TCPClient(string server, int port, int &status, int domain = AF_UNSPEC /* Don't care IPv4 or IPv6 */, int maxBufLen = 1e6)
        : sockfd{-1}, port{port}, serverIP{""}, serverAddr{nullptr}, serverInfo{nullptr}, MAX_BUFF_LEN{maxBufLen} {
        // Mark the status as -1 to denote error
        status = -1;

        // The structs required for providing hints
        struct addrinfo hints;

        // Zero out the struct for providing hints
        memset(&hints, 0, sizeof(hints));

        hints.ai_family = domain;         // AF_INET or AF_INET6
        hints.ai_socktype = SOCK_STREAM;  // For TCP socket

        int rv = 0;  // Return value

        // Get the server address info
        if ((rv = getaddrinfo(server.c_str(), to_string(port).c_str(), &hints, &serverInfo)) != 0) {
            std::cerr << "client: getaddrinfo: " << gai_strerror(rv) << "\n";
            return;
        }

        // loop through all the results and bind to the first we can
        for (serverAddr = serverInfo; serverAddr != nullptr; serverAddr = serverAddr->ai_next) {
            // Create a socket with the same domain, type and protocol as used by the server
            if ((sockfd = socket(serverAddr->ai_family, serverAddr->ai_socktype, serverAddr->ai_protocol)) == -1) {
                perror("client: socket");
                continue;
            }

            // Connect to the server
            if (connect(sockfd, serverAddr->ai_addr, serverAddr->ai_addrlen) == -1) {
                close(sockfd);
                perror("client: connect");
                continue;
            }

            // Break as we make the first socket from the results and connected to the server
            break;
        }

        // If while iterating on linked list, we reach the end, then we are unable to create a socket
        if (serverAddr == nullptr) {
            std::cerr << "client: failed to bind\n";
            return;
        }

        // Set server IP address
        serverIP = getIP(serverAddr->ai_addr);

        // Mark the return status as 0 to denote success
        status = 0;

        std::cout << "client: successfully connected to " << serverIP << ":" << port << "\n";
    }

    // Destructor
    ~TCPClient() {
        // Close the socket file descriptor
        if (sockfd != -1) close(sockfd);

        // Release the resources for the server info and the internal linked lis
        if (serverInfo != nullptr) freeaddrinfo(serverInfo);
    }

    // Function to get the socket file descriptor for communicating with the server
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

    // Function to send data to the server
    int sendData(string data, int flags = 0) {
        if (sockfd == -1) return -1;

        // Send data to the client
        if (send(sockfd, data.c_str(), data.length(), flags) == -1) {
            perror("client: send");
            return -1;
        }

        return 0;
    }

    // Function to receive data from the server
    string receiveData(int flags = 0) {
        if (sockfd == -1) throw "Invalid socket";

        // The number of bytes read from the server
        int numBytes = 0;

        // Buffer to read the input message received from the server
        char buff[MAX_BUFF_LEN];
        if ((numBytes = recv(sockfd, buff, MAX_BUFF_LEN - 1, flags)) == -1) {
            perror("client: recv");
            throw "Unable to receive data";
        }

        // Mark the end of the input message with NULL character to indicate end of string
        buff[numBytes] = '\0';

        return string(buff);
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

int main() {
    int status = 0;
    // Create a TCP client
    TCPClient client("localhost", PORT, status);
    if (status == -1) {
        exit(1);
    }

    // Send data to the server
    if (client.sendData("Hi from client!") == -1) {
        exit(1);
    }

    // Read data from the server
    try {
        std::cout << "Message from server: " << client.receiveData() << "\n";
    } catch (exception &ex) {
        exit(1);
    }

    return 0;
}