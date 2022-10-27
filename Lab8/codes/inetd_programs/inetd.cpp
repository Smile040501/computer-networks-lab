// 111901030
// Mayank Singla

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
#include <vector>
using namespace std;

using ll = long long;

#define H2_IP       "192.168.1.3"
#define ADD_PORT    12500  // Port on which the server is listening
#define SUB_PORT    12501
#define MUL_PORT    12502
#define IDIV_PORT   12503
#define ADD_EXE     "/home/tc/inetd_add"
#define SUB_EXE     "/home/tc/inetd_sub"
#define MUL_EXE     "/home/tc/inetd_mul"
#define IDIV_EXE    "/home/tc/inetd_idiv"
#define H3_IP       "192.168.1.4"
#define H3_PORT     4950
#define DOMAIN      AF_INET
#define DELIMITER   '|'
#define INVALID_STR "__INVALID__"

#define BACKLOG 20  // How many pending connections queue will hold

// TCPServer class to create a TCP server
class TCPServer {
    ll sockfd;                                 // Socket file descriptor on which the server will listen
    ll port;                                   // Port on which the server will be bound to
    string myIP;                               // IP address of the server
    struct addrinfo *serverAddr, *serverInfo;  // Server's address information
    const ll MAX_BUFF_LEN;                     // Maximum buffer length while receiving the data

   public:
    // Constructor
    TCPServer(ll port, ll &status, ll domain = AF_UNSPEC /* Don't care IPv4 or IPv6 */, ll maxBufLen = 1e6)
        : sockfd{-1}, port{port}, myIP{""}, serverAddr{nullptr}, serverInfo{nullptr}, MAX_BUFF_LEN{maxBufLen} {
        // Mark the status as -1 to denote error
        status = -1;

        // The structs required for providing hints
        struct addrinfo hints;

        // Zero out the struct for providing hints
        memset(&hints, 0, sizeof(hints));

        hints.ai_family = domain;         // AF_INET or AF_INET6
        hints.ai_socktype = SOCK_STREAM;  // For TCP socket
        hints.ai_flags = AI_PASSIVE;      // Use my IP

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

        std::cout << "server: successfully created a TCP socket at " << myIP << ":" << port << "\n";
    }

    // Destructor
    ~TCPServer() {
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

    // Function to start the server for listening for connections on the port
    ll startListening(ll backlogQSize) {
        if (sockfd == -1) return -1;

        // start listening on the socket file descriptor
        if (listen(sockfd, backlogQSize) == -1) {
            perror("server: listen");
            return -1;
        }

        std::cout << "server: waiting for connections at " << myIP << ":" << port << "\n";
        return 0;
    }

    // Function to accept the incoming connections from the client
    ll acceptConnection(struct sockaddr_storage *clientAddr /* Clients's address information */) {
        if (sockfd == -1) return -1;

        // The new socket file descriptor which will be used for communication with this client
        ll newFd = -1;

        // Size of the struct
        socklen_t addrLen = sizeof(*clientAddr);

        // Try accepting the connection from the client
        if ((newFd = accept(sockfd, (struct sockaddr *)clientAddr, &addrLen)) == -1) {
            perror("server: accept");
            return -1;
        }

        std::cout << "server: got connection from " << getIP((struct sockaddr *)clientAddr) << ":" << getPort((struct sockaddr *)clientAddr) << "\n";
        return newFd;
    }

    // Function to send data to the client given its socket file descriptor to communicate
    ll sendData(ll fd, string data, ll flags = 0) {
        if (sockfd == -1) return -1;

        ll len = data.length();  // Length of the data to send
        ll total = 0;            // How many bytes we've sent
        ll bytesLeft = len;      // How many bytes we are left to send

        // While we have not fully sent the data to the client
        while (total < len) {
            // Send the remaining to the client
            ll numBytes = send(fd, data.c_str() + total, bytesLeft, flags);
            if (numBytes == -1) {
                perror("server: send");
                return -1;
            }
            // Update the variables
            total += numBytes;
            bytesLeft -= numBytes;
        }

        return 0;
    }

    // Function to receive data from the client given its socket file descriptor to communicate
    string receiveData(ll fd, ll flags = 0) {
        if (sockfd == -1) throw "Invalid socket";

        // The number of bytes read from the client
        ll numBytes = 0;

        // Buffer to read the input message received from the client
        char buff[MAX_BUFF_LEN];
        if ((numBytes = recv(fd, buff, MAX_BUFF_LEN - 1, flags)) == -1) {
            perror("server: recv");
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

// TCPService class to represent a TCP service
class TCPService {
   public:
    string name;        // Name of the TCP service
    ll port;            // Port on which the service is listening
    string pathToExec;  // Path to the executable for the service
    TCPServer *server;  // Server object created for the service

    // Constructor
    TCPService(string name, ll port, string pathToExec)
        : name{name}, port{port}, pathToExec{pathToExec} {}
};

// Creates all the services which inetd daemon has to monitor
vector<TCPService> createServices() {
    vector<TCPService> services;
    services.emplace_back(TCPService("inetd_add", ADD_PORT, ADD_EXE));
    services.emplace_back(TCPService("inetd_sub", SUB_PORT, SUB_EXE));
    services.emplace_back(TCPService("inetd_mul", MUL_PORT, MUL_EXE));
    services.emplace_back(TCPService("inetd_idiv", IDIV_PORT, IDIV_EXE));
    return services;
}

int main() {
    fd_set master;   // master file descriptor list for select()
    fd_set readFds;  // temp file descriptor list for select()
    ll fdMax = -1;   // maximum file descriptor number

    FD_ZERO(&master);   // clear the master sets
    FD_ZERO(&readFds);  // clear the temp sets

    ll status = 0;  // The success status

    // Create all the services
    vector<TCPService> services = createServices();

    // Creating a TCP server for each service
    for (TCPService &s : services) {
        s.server = new TCPServer(s.port, status, DOMAIN);
        if (status == -1) exit(EXIT_FAILURE);
    }

    // Start listening on the server of each service
    for (const TCPService &s : services) {
        if (s.server->startListening(BACKLOG) == -1) exit(EXIT_FAILURE);
    }

    // Add the socket file descriptor of all the listeners to the master set
    for (const TCPService &s : services) {
        ll sockfd = s.server->getSockFD();
        FD_SET(sockfd, &master);

        // Find the maximum file descriptor
        fdMax = max(fdMax, sockfd);
    }

    for (;;) {
        // Initialize the temp set
        readFds = master;

        // Wait of any file descriptor to become ready to read
        // A TCP socket file descriptor will become ready to read when it is ready to accept connection
        if (select(fdMax + 1, &readFds, nullptr, nullptr, nullptr) == -1) {
            perror("select");
            exit(EXIT_FAILURE);
        }

        // Check which socket file descriptor has become ready to read
        for (const TCPService &s : services) {
            if (FD_ISSET(s.server->getSockFD(), &readFds)) {
                std::cout << "Selecting the service: " << s.name << "\n";

                // `sockaddr_storage` struct populate the client address from which the packet is received
                struct sockaddr_storage clientAddr;
                ll newFd = 0;  // Socket file descriptor for communicating with client (h1)

                // Accept connection from the client (h1)
                if ((newFd = s.server->acceptConnection(&clientAddr)) == -1) exit(EXIT_FAILURE);

                // Find the IP address of the client (h1)
                string clientIP = TCPServer::getIP((struct sockaddr *)&clientAddr);

                if (!fork()) {  // this is the child process
                    // Overwriting the STDIN with connection socket file descriptor
                    dup2(newFd, STDIN_FILENO);

                    // Not overwriting the STDOUT and STDERR
                    // as I want my sub services to print to the terminal

                    // Create new pointers for the path and the IP address of the client in child process
                    char *path = new char[s.pathToExec.length()];
                    char *ip = new char[clientIP.length()];
                    strcpy(path, s.pathToExec.c_str());
                    strcpy(ip, clientIP.c_str());

                    // Arguments array for the `execv` system call
                    char *args[] = {path, ip, NULL};

                    // Execute the service and provide the client(H1)'s IP address as the argument
                    ll r = execv(args[0], args);
                }

                close(newFd);  // parent doesn't need this
            }
        }
    }

    return EXIT_SUCCESS;
}
