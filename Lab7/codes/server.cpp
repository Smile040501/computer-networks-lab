// 111901030
// Mayank Singla

#include <arpa/inet.h>
#include <netdb.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

#include <bitset>
#include <cstring>
#include <iostream>
#include <map>
#include <vector>

using namespace std;
using ll = long long;

#define MAX_BITS          8        // For 8-bit binary string
#define PRE_PRECODE_BITS  4        // Number of bits before 4B/5B encoding
#define POST_PRECODE_BITS 5        // Number of bits after 4B/5B encoding
#define FRAME_DELIMITER   "00001"  // Reserved flag from unused 5B codes
#define MY_PORT           "9567"   // The port on which the server is listening
#define MAX_BUFF_LEN      1000000  // Maximum frame size allowed to be sent

// 4B-5B reverse code mapping
map<string, string> b5tob4 = {
    {"11110", "0000"},
    {"01001", "0001"},
    {"10100", "0010"},
    {"10101", "0011"},
    {"01010", "0100"},
    {"01011", "0101"},
    {"01110", "0110"},
    {"01111", "0111"},
    {"10010", "1000"},
    {"10011", "1001"},
    {"10110", "1010"},
    {"10111", "1011"},
    {"11010", "1100"},
    {"11011", "1101"},
    {"11100", "1110"},
    {"11101", "1111"}};

// Function to get the `struct sockaddr_in` or `struct sockaddr_in6`
// depending upon IPv4 or IPv6 type input `sockaddr`
void *get_in_addr(struct sockaddr *sa) {
    // AF_INET is for IPv4
    if (sa->sa_family == AF_INET) {
        return &(((struct sockaddr_in *)sa)->sin_addr);
    }

    return &(((struct sockaddr_in6 *)sa)->sin6_addr);
}

// Function to check if a given string starts with another given string
bool startsWith(const string &s, const string &t) {
    return s.find(t) == 0;
}

// Function to check if a given string ends with another given string
bool endsWith(const string &s, const string &t) {
    return s.rfind(t) == (s.length() - t.length());
}

// Function to convert a binary string to its decimal value
ll binToDec(const string &s) {
    // The input binary string is expected to be of MAX_BITS length
    bitset<MAX_BITS> b(s);
    return b.to_ullong();
}

// Function to correct the input hamming code having 1-bit error
string correctHammingCode(const string &s) {
    ll strLen = s.length();  // length of the input code

    // Populate the code vector with input binary values 1 or 0
    vector<ll> code{0};
    for (ll i = 0; i < strLen; ++i) {
        code.emplace_back(s[i] - '0');
    }

    ll sumBadBitsIdx = 0;  // Sum of the index parity positions where the bit was faulty

    // Computing the parity bit again for the index positions which are powers of 2
    for (ll i = 1; i < code.size(); ++i) {
        // If idx is not power of 2, then skip
        if ((i & (i - 1)) != 0) continue;

        // Counting the number of 1 bits starting from current position, taking
        // i positions, then alternately skipping and taking i positions
        ll countOne = 0;
        for (ll j = i; j < code.size(); j += i) {
            for (ll k = 0; k < i && j < code.size(); ++k, ++j) {
                if (j == i) continue;
                if (code[j] == 1) ++countOne;
            }
        }

        // If new parity is not equal to the received parity, then it is faulty
        if ((countOne % 2 == 0 && code[i] != 0) || (countOne % 2 == 1 && code[i] != 1)) {
            // Adding the index to the sum
            sumBadBitsIdx += i;
        }
    }

    // The parity bit as the final sum is faulty and we need to flip that bit
    code[sumBadBitsIdx] ^= 1;

    string ans = "";
    // Join the corrected string to get the final corrected hamming code
    for (ll i = 1; i < code.size(); ++i) {
        ans += ('0' + code[i]);
    }
    return ans;
}

// Function to extract the message bits from the input hamming code
string extractMsg(const string &s) {
    ll strLen = s.length();  // The length of the input code

    // Populate the code vector with input binary values 1 or 0
    vector<ll> code{0};
    for (ll i = 0; i < strLen; ++i) {
        code.emplace_back(s[i] - '0');
    }

    string ans = "";
    // Taking out the message bits which are at the index positions which are not powers of 2
    for (ll i = 1; i < code.size(); ++i) {
        // Skip if index is power of 2
        if ((i & (i - 1)) == 0) continue;
        ans += ('0' + code[i]);
    }
    return ans;
}

// Function to de-frame the input message
string deFrameMsg(const string &s) {
    if (!startsWith(s, FRAME_DELIMITER) || !endsWith(s, FRAME_DELIMITER)) {
        // This condition should never occur as per our previous implementation
        // hence, this code should never be reached
        throw "deFrameMsg: Unexpected Implementation Error";
    }

    // Return the substring between the starting and ending frame delimeter
    return s.substr(POST_PRECODE_BITS, s.length() - (2 * POST_PRECODE_BITS));
}

// Function to decode the input message using the reverse encoding for 4B/5B encoding
string decodeMsg(const string &s) {
    ll strLen = s.length();  // The given string length
    if (strLen % POST_PRECODE_BITS != 0) {
        // This condition should never occur as per our previous implementation
        // hence, this code should never be reached
        throw "decodeMsg: Unexpected Implementation Error";
    }

    string ans = "";
    for (ll i = 0; i < strLen; i += POST_PRECODE_BITS) {
        // Take the group of `POST_PRECODE_BITS` characters as a key for the map
        string key = s.substr(i, POST_PRECODE_BITS);

        // Append the encoded value to the result
        ans += b5tob4[key];
    }
    return ans;
}

// Function to print the final received message by converting the input binary string
// to its ASCII representation
void printMsg(const string &s) {
    ll strLen = s.length();  // The input string length
    if (strLen % MAX_BITS != 0) {
        // This condition should never occur as per our previous implementation
        // hence, this code should never be reached
        throw "printMsg: Unexpected Implementation Error";
    }

    std::cout << "Message Received: ";
    for (ll i = 0; i < strLen; i += MAX_BITS) {
        // Take the group of MAX_BITS at a time
        ll dec = binToDec(s.substr(i, MAX_BITS));

        // If decimal value is 0, then skip as they are the padding bits
        if (dec == 0) continue;

        // The character representing the decimal value
        char ch = dec;
        std::cout << ch;
    }
    // Printing the new line at the end
    std::cout << "\n";
}

int main() {
    // The structs required for providing hints and getting the server info
    struct addrinfo hints, *serverInfo;

    // Zero out the struct for providing hints
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET;       // set to AF_INET to use IPv4
    hints.ai_socktype = SOCK_DGRAM;  // for UDP sockets
    hints.ai_flags = AI_PASSIVE;     // use my IP

    ll rv = 0;  // Return value
    // Get the server port info using port
    if ((rv = getaddrinfo(nullptr, MY_PORT, &hints, &serverInfo)) != 0) {
        std::cerr << "server: getaddrinfo: " << gai_strerror(rv) << "\n";
        return EXIT_FAILURE;
    }

    // loop through all the results, and make and bind to the first socket we can
    struct addrinfo *servAddr;
    ll sockfd = 0;  // socket file descriptor
    for (servAddr = serverInfo; servAddr != nullptr; servAddr = servAddr->ai_next) {
        // Create a socket with the same domain, type and protocol as used by the server
        if ((sockfd = socket(servAddr->ai_family, servAddr->ai_socktype, servAddr->ai_protocol)) == -1) {
            perror("server: socket");
            continue;
        }

        // Bind the socket
        if (bind(sockfd, servAddr->ai_addr, servAddr->ai_addrlen) == -1) {
            // Close the socket file descriptor
            close(sockfd);
            perror("server: bind");
            continue;
        }

        // Break as we make and bind the first socket from the results
        break;
    }

    // If while iterating on linked list, we reach the end, then we are unable to create a socket
    if (servAddr == nullptr) {
        std::cerr << "server: Failed to bind socket\n";
        return EXIT_FAILURE;
    }

    // Release the resources for the server info and the internal linked list
    freeaddrinfo(serverInfo);

    std::cout << "UDP server up on the port " << MY_PORT << "...\n";

    // `sockaddr_storage` struct populate the client address from which the packet is received
    struct sockaddr_storage clientAddr;

    // Buffer to read the input message received from the client
    char buff[MAX_BUFF_LEN];

    // Size of the client address
    socklen_t addrLen = sizeof(clientAddr);

    // Buffer to populate the IPv4 address of the client
    char ip4[INET_ADDRSTRLEN];

    // The number of bytes read from the client
    ll numBytes = 0;

    // The final binary message received from the client
    string msg = "";

    // Indicates whether the frame is received in the input binary message or not
    bool startFrame = false;

    // Keep on listening for the packets forever
    while (true) {
        // Read the input faulty hamming code
        if ((numBytes = recvfrom(sockfd, buff, MAX_BUFF_LEN - 1, 0, (struct sockaddr *)&clientAddr, &addrLen)) == -1) {
            perror("server: recvfrom");
            return EXIT_FAILURE;
        }
        // Mark the end of the input message with NULL character to indicate end of string
        buff[numBytes] = '\0';

        // The faulty hamming code
        string code = buff;

        // Get the corrected hamming code
        string correctedCode = correctHammingCode(code);

        std::cout << "Got packet from \"" << inet_ntop(clientAddr.ss_family, get_in_addr((struct sockaddr *)&clientAddr), ip4, sizeof(ip4)) << "\"\n"
                  << "Packet is " << numBytes << " bytes long\n"
                  << "Packet contains \"" << buff << "\"\n"
                  << "Corrected Packet \"" << correctedCode << "\"\n\n";

        // Extract the message bits from the corrected code
        string subMsg = extractMsg(correctedCode);

        // Append it to the final message string
        msg += subMsg;

        // Length of the message string so far
        ll msgLen = msg.length();

        // If frame is not yet received and the length of the message received
        // is greater than the frame length
        if (!startFrame && msgLen >= POST_PRECODE_BITS) {
            // Check if the frame is present in the final message or not
            if (!startsWith(msg, FRAME_DELIMITER)) {
                // Packet containing the frame bits should arrive first
                // This condition should never occur as per our previous implementation
                // hence, this code should never be reached
                throw "main: Packet Disordering";
            }

            // Mark the frame start as received
            startFrame = true;
        }

        // If frame is started and length of the input message is greater than twice the frame length
        // Check if the message received so far ends with the frame delimeter
        // If yes, then we have successfully received one whole message
        if (startFrame && (msgLen >= 2 * POST_PRECODE_BITS) && endsWith(msg, FRAME_DELIMITER)) {
            // Mark the frame start as false to start a new message
            startFrame = false;

            // Get the de-framed message
            string deFramedMsg = deFrameMsg(msg);

            // Get the decoded message using reverse 4B-5B encoding
            string decodedMsg = decodeMsg(deFramedMsg);

            // Print the final message received in its ASCII representation
            printMsg(decodedMsg);

            // Clear the message to start a new message
            msg = "";
        }
    }

    // Close the socket file descriptor
    close(sockfd);

    return EXIT_SUCCESS;
}
