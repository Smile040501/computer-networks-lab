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
#include <sstream>
using namespace std;

using ll = long long;

#define H3_IP       "192.168.1.4"  // H3 IP address
#define H3_PORT     4950           // Port on which H3 will listen for data from H2
#define DOMAIN      AF_INET        // IPv4 domain to use
#define DELIMITER   '|'            // Delimiter for destructuring the message received from H2
#define INVALID_STR "__INVALID__"  // If message received is invalid
#define BACKLOG     20             // How many pending connections queue will hold

// UDPServer class to create a UDP server
class UDPServer {
    ll sockfd;                                 // Socket file descriptor on which the server will listen
    ll port;                                   // Port on which the server will be bound to
    string myIP;                               // IP address of the server
    struct addrinfo *serverAddr, *serverInfo;  // Server's address information
    const ll MAX_BUFF_LEN;                     // Maximum buffer length while receiving the data

   public:
    // Constructor
    UDPServer(ll port, ll &status, ll domain = AF_UNSPEC /* Don't care IPv4 or IPv6 */, ll maxBufLen = 1e6)
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

        ll rv = 0;  // Return value

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
            ll yes = 1;
            if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(yes)) == -1) {
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
    ll getSockFD() {
        return sockfd;
    }

    void setSockFD(ll fd) {
        sockfd = fd;
    }

    // Function to get the port on which the server is listening
    ll getPort() {
        return port;
    }

    string getIP() {
        return myIP;
    }

    // Function to get the address information of the server
    addrinfo getServerAddrInfo() {
        return *serverAddr;
    }

    // Function to send data to the client
    ll sendData(string data, struct sockaddr_storage *clientAddr, ll flags = 0) {
        if (sockfd == -1) return -1;

        // Length of the struct
        socklen_t addrLen = sizeof(*clientAddr);

        ll len = data.length();  // Length of the data to send
        ll total = 0;            // How many bytes we've sent
        ll bytesLeft = len;      // How many bytes we are left to send

        // While we have not fully sent the data to the client
        while (total < len) {
            // Send the remaining to the client
            ll numBytes = sendto(sockfd, data.c_str() + total, bytesLeft, flags, (struct sockaddr *)clientAddr, addrLen);
            if (numBytes == -1) {
                perror("server: sendto");
                return -1;
            }
            // Update the variables
            total += numBytes;
            bytesLeft -= numBytes;
        }

        return 0;
    }

    // Function to receive data from the client
    pair<string, struct sockaddr_storage *> receiveData(ll flags = 0) {
        if (sockfd == -1) throw "Invalid socket";

        // The number of bytes read from the client
        ll numBytes = 0;

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

// TCPClient class to create a TCP client
class TCPClient {
    ll sockfd;                                 // Socket file descriptor which the client will use to communicate
    ll port;                                   // Port on which the server is listening
    string serverIP;                           // IP address of the server
    struct addrinfo *serverAddr, *serverInfo;  // Server's address information
    const ll MAX_BUFF_LEN;                     // Maximum buffer length while receiving the data

   public:
    // Constructor
    TCPClient(string server, ll port, ll &status, ll domain = AF_UNSPEC /* Don't care IPv4 or IPv6 */, ll maxBufLen = 1e6)
        : sockfd{-1}, port{port}, serverIP{""}, serverAddr{nullptr}, serverInfo{nullptr}, MAX_BUFF_LEN{maxBufLen} {
        // Mark the status as -1 to denote error
        status = -1;

        // The structs required for providing hints
        struct addrinfo hints;

        // Zero out the struct for providing hints
        memset(&hints, 0, sizeof(hints));

        hints.ai_family = domain;         // AF_INET or AF_INET6
        hints.ai_socktype = SOCK_STREAM;  // For TCP socket

        ll rv = 0;  // Return value

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
    ll getSockFD() {
        return sockfd;
    }

    void setSockFD(ll fd) {
        sockfd = fd;
    }

    // Function to get the port on which the server is listening
    ll getPort() {
        return port;
    }

    string getServerIP() {
        return serverIP;
    }

    // Function to get the address information of the server
    addrinfo getServerAddrInfo() {
        return *serverAddr;
    }

    // Function to send data to the server
    ll sendData(string data, ll flags = 0) {
        if (sockfd == -1) return -1;

        ll len = data.length();  // Length of the data to send
        ll total = 0;            // How many bytes we've sent
        ll bytesLeft = len;      // How many bytes we are left to send

        // While we have not fully sent the data
        while (total < len) {
            // Send the remaining
            ll numBytes = send(sockfd, data.c_str() + total, bytesLeft, flags);
            if (numBytes == -1) {
                perror("client: send");
                return -1;
            }
            // Update the variables
            total += numBytes;
            bytesLeft -= numBytes;
        }

        return 0;
    }

    // Function to receive data from the server
    string receiveData(ll flags = 0) {
        if (sockfd == -1) throw "Invalid socket";

        // The number of bytes read from the server
        ll numBytes = 0;

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

// Handler for SIGCHLD exception
void sigchld_handler(int s) {
    // waitpid() might overwrite errno, so we save and restore it:
    int saved_errno = errno;

    while (waitpid(-1, NULL, WNOHANG) > 0)
        ;

    errno = saved_errno;
}

// Function to extract the required fields from the message received from H2
// IP|PORT|RES
void extractData(const string &data, string &ip, ll &port, string &result) {
    // Create a string stream object
    istringstream ss(data);
    string token = "";
    ll numTokens = 0, res = 0;
    // Splitting the string by the DELIMITER
    // IP|PORT|RES
    while (getline(ss, token, DELIMITER)) {
        ++numTokens;
        if (numTokens == 1) {
            ip = token;
        } else if (numTokens == 2) {
            port = stoll(token);
        } else if (numTokens == 3) {
            res = stoll(token);
        } else {
            break;
        }
    }
    if (numTokens != 3) {
        // Message validation failed
        result = INVALID_STR;
        return;
    }
    // Final result will be the squared number
    res *= res;
    result = to_string(res);
}

int main() {
    ll status = 0;  // The success status
    // Creating a UDP server (for h2 -> h3)
    UDPServer server(H3_PORT, status, DOMAIN);
    if (status == -1) exit(EXIT_FAILURE);

    // waitpid() might overwrite errno, so we save and restore it by providing our handler
    struct sigaction sa;
    sa.sa_handler = sigchld_handler;  // reap all dead processes
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = SA_RESTART;
    if (sigaction(SIGCHLD, &sa, NULL) == -1) {
        perror("sigaction");
        exit(EXIT_FAILURE);
    }

    while (1) {
        try {
            // Read data from the client (h2)
            auto p = server.receiveData();

            std::cout << "server: got packet from " << UDPServer::getIP((struct sockaddr *)p.second) << ":" << UDPServer::getPort((struct sockaddr *)p.second) << "\n";

            std::cout << "server: packet contains \"" << p.first << "\"\n";

            // Get the IP, PORT and Result from the message
            string ip = "", result = "";
            ll port = 0;
            extractData(p.first, ip, port, result);

            // Create a TCP client (for h3 -> h1)
            TCPClient client(ip, port, status, DOMAIN);
            if (status == -1) exit(EXIT_FAILURE);

            // Send data to the server (h1)
            if (client.sendData(result) == -1) exit(EXIT_FAILURE);
        } catch (exception &ex) {
            exit(EXIT_FAILURE);
        }
    }

    return EXIT_SUCCESS;
}
