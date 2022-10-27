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

using ll = long long;

#define H1_IP     "192.168.1.2"   // H1 IP address
#define H1_PORT   3490            // Port on which H1 is listening for H3
#define H2_IP     "192.168.1.3"   // H2 IP address
#define ADD_PORT  12500           // Port on which the add service is listening on H2
#define SUB_PORT  12501           // Port on which the sub service is listening on H2
#define MUL_PORT  12502           // Port on which the mul service is listening on H2
#define IDIV_PORT 12503           // Port on which the idiv service is listening on H2
#define BACKLOG   20              // How many pending connections queue will hold
#define DOMAIN    AF_INET         // IPv4 domain to use
#define ADD_OP    string("ADD")   // Operation for add to be specified from command line
#define SUB_OP    string("SUB")   // Operation for sub to be specified from command line
#define MUL_OP    string("MUL")   // Operation for mul to be specified from command line
#define IDIV_OP   string("IDIV")  // Operation for idiv to be specified from command line
#define DELIMITER '|'             // Delimiter for constructing the message to be sent to H2

// The expected number of input arguments
#define EXP_ARGS 3

// The argument string to execute the program
#define ARGS_STR "./h1 <num1> <num2> <" + ADD_OP + "/" + SUB_OP + "/" + MUL_OP + "/" + IDIV_OP + ">"

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

// Takes a string as input and convert it to a long long integer
// If the string contains non-digit characters, it will throw error
ll stringToLong(const string &s) {
    bool isNeg = false;
    if (s[0] == '-') isNeg = true;  // whether the number is negative or not

    ll ans = 0;
    for (ll i = 0 + isNeg; i < s.length(); ++i) {
        if (!isdigit(s[i])) throw exception();
        // construct the number
        ans *= 10;
        ans += (s[i] - '0');
    }
    return isNeg ? -ans : ans;
}

// Function to end the program on receiving invalid arguments from the command line
void endProg() {
    std::cerr << "INVALID ARGUMENTS\n";
    std::cerr << "Please provide the arguments as follows: " << ARGS_STR << "\n";
    exit(EXIT_FAILURE);
}

// Function to construct the required string format to be sent to H2
// PORT|NUM1|NUM2
string constructString(ll num1, ll num2) {
    return to_string(H1_PORT) + DELIMITER + to_string(num1) + DELIMITER + to_string(num2);
}

// Handler for SIGCHLD exception
void sigchld_handler(int s) {
    // waitpid() might overwrite errno, so we save and restore it:
    int saved_errno = errno;

    while (waitpid(-1, NULL, WNOHANG) > 0)
        ;

    errno = saved_errno;
}

int main(int argc, char const *argv[]) {
    if (argc != EXP_ARGS + 1) {
        // This program requires EXP_ARGS arguments from the command line
        std::cerr << "Expected " << EXP_ARGS << " arguments, but got " << argc - 1 << "\n";
        endProg();
    }

    // Reading the numbers from the command line
    ll num1 = 0, num2 = 0;
    try {
        num1 = stringToLong(argv[1]);
        num2 = stringToLong(argv[2]);
    } catch (exception &e) {
        endProg();
    }

    // The operation that needs to be performed
    string op = argv[3];

    // Find the corresponding port based on the operation
    ll port = 0;
    if (op == ADD_OP) {
        port = ADD_PORT;
    } else if (op == SUB_OP) {
        port = SUB_PORT;
    } else if (op == MUL_OP) {
        port = MUL_PORT;
    } else if (op == IDIV_OP) {
        port = IDIV_PORT;
    } else {
        // INVALID OPERATION
        endProg();
    }

    ll status = 0;  // The success status
    // Creating a TCP Server (for h3 -> h1)
    TCPServer server(H1_PORT, status, DOMAIN);
    if (status == -1) exit(EXIT_FAILURE);

    // Start listening on the server
    if (server.startListening(BACKLOG) == -1) exit(EXIT_FAILURE);

    // Create a TCP client (for h1 -> h2)
    TCPClient client(H2_IP, port, status, DOMAIN);
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

    // Send data to the server (h2)
    if (client.sendData(constructString(num1, num2)) == -1) exit(EXIT_FAILURE);

    // `sockaddr_storage` struct populate the client address from which the packet is received
    struct sockaddr_storage clientAddr;
    int newFd = 0;  // New socket file descriptor for communicating with client (h3)

    // Accept connections from the client (h3)
    if ((newFd = server.acceptConnection(&clientAddr)) == -1) exit(EXIT_FAILURE);

    // Read data from the client (h3)
    string data = "";
    try {
        data = server.receiveData(newFd);
        std::cout << "Message from client: " << data << "\n";
        std::cout << "The result obtained: " << stoll(data) << "\n";
    } catch (exception &ex) {
        exit(EXIT_FAILURE);
    }

    // Close all the file descriptors
    close(newFd);

    return EXIT_SUCCESS;
}
