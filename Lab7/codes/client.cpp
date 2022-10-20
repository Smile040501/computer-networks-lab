// 111901030
// Mayank Singla

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

#define EXP_ARGS          2                            // Expected number of input arguments
#define ARGS_STR          "./client <msg> <msg_bits>"  // Argument string to execute the program
#define MAX_BITS          8                            // For 8-bit binary string
#define PRE_PRECODE_BITS  4                            // Number of bits before 4B/5B encoding
#define POST_PRECODE_BITS 5                            // Number of bits after 4B/5B encoding
#define FRAME_DELIMITER   "00001"                      // Reserved flag from unused 5B codes
#define IP                "192.168.1.2"                // IP address of the machine to send
#define PORT              "8567"                       // Port on which the machine is listening
#define MAX_BUFF_LEN      1000000                      // Maximum frame size allowed to be sent

// 4B-5B code mapping
map<string, string> b4tob5 = {
    {"0000", "11110"},
    {"0001", "01001"},
    {"0010", "10100"},
    {"0011", "10101"},
    {"0100", "01010"},
    {"0101", "01011"},
    {"0110", "01110"},
    {"0111", "01111"},
    {"1000", "10010"},
    {"1001", "10011"},
    {"1010", "10110"},
    {"1011", "10111"},
    {"1100", "11010"},
    {"1101", "11011"},
    {"1110", "11100"},
    {"1111", "11101"}};

// Function to calculate the gcd of two numbers
ll gcd(ll a, ll b) {
    if (b == 0) {
        return a;
    }
    return gcd(b, a % b);
}

// Function to get the minimum value of `r` that satisfies
// (m + r + 1) <= 2^r
ll getMinNoRedundantBits(ll m) {
    ll r = 1;
    while ((m + r + 1) > (1 << r)) {
        ++r;
    }
    return r;
}

// Function to encode the input string in its binary representation
// using the ASCII value of its characters and MAX_BITS per character
string getBinaryMsg(const string &s) {
    string ans = "";
    // Iterate over each characeter
    for (const auto &c : s) {
        // Using a `bitset` to get the binary string of its ASCII value
        bitset<MAX_BITS> b(c - 0);
        ans += b.to_string();
    }
    return ans;
}

/*
    Function to get the minimum number of bits that needs to be padded to
    binary string of given length and given number of message bits in the frame

    Let say the input length of the binary string is s and we want to pad x bits
    -   We want (s + x) to be divisible by PRE_PRECODE_BITS, so that they can nicely be grouped for encoding
    -   We want (s + x) to be divisible by MAX_BITS, so that they can nicely be decoded
    -   We want ((((s + x) / PRE_PRECODE_BITS) * POST_PRECODE_BITS) + POST_PRECODE_BITS + POST_PRECODE_BITS) to be divisible by `msgBits` so that they can nicely be grouped for framing
*/
ll getMinPadBits(ll strLen, ll msgBits) {
    // Get the lcm of PRE_PRECODE_BITS and MAX_BITS
    ll lcm = (PRE_PRECODE_BITS / gcd(PRE_PRECODE_BITS, MAX_BITS)) * MAX_BITS;

    // Nearest length from `strLen` which is divisible by the lcm computed
    ll prePrecodeLen = strLen + ((lcm - (strLen % lcm)) % lcm);

    // Loop until we get the minimum value
    while (true) {
        // The length which will be after encoding and framing
        ll postPrecodeLen = ((prePrecodeLen / PRE_PRECODE_BITS) * POST_PRECODE_BITS) + (2 * POST_PRECODE_BITS);

        // Check if it is divisible by `msgBits`
        if (postPrecodeLen % msgBits == 0) {
            break;
        }
        // Jump to the next suitable value
        prePrecodeLen += lcm;
    }
    // The number of padded bits
    return prePrecodeLen - strLen;
}

// Function to pad the given binary string by `0` with the number of given padding bits
string padBinaryString(const string &s, ll padBits) {
    string ans = s;
    for (ll i = 0; i < padBits; ++i) {
        ans = "0" + ans;
    }
    return ans;
}

// Function to encode the given string using 4B/5B encoding
string preCodeMsg(const string &s) {
    // The length of the string
    ll strLen = s.length();
    if (strLen % PRE_PRECODE_BITS != 0) {
        // This condition should never occur as per our previous implementation
        // hence, this code should never be reached
        throw "preCodeMsg: Unexpected Implementation Error";
    }

    string ans = "";
    for (ll i = 0; i < strLen; i += PRE_PRECODE_BITS) {
        // Take the group of `PRE_PRECODE_BITS` characters as a key for the map
        string key = s.substr(i, PRE_PRECODE_BITS);

        // Append the encoded value to the result
        ans += b4tob5[key];
    }
    return ans;
}

// Function to frame the given string with the `FRAME_DELIMITER` at the start and the end
string frameMsg(const string &s) {
    return FRAME_DELIMITER + s + FRAME_DELIMITER;
}

// Function to get the hamming code for the given binary string
string getHammingCode(const string &s, ll msgBits, ll redBits) {
    ll strLen = s.length(), countRedBits = 0;
    if (strLen != msgBits) {
        // This condition should never occur as per our previous implementation
        // hence, this code should never be reached
        throw "getHammingCode: Unexpected Implementation Error";
    }

    // Spreading the message bits on index positions which are not powers of 2
    vector<ll> code{0};
    for (ll i = 1, j = 0; j < strLen; ++i) {
        // If index is a power of 2
        if ((i & (i - 1)) == 0) {
            // Just putting a temporary value for now
            code.emplace_back(0);

            // These positions are for the check/redundant bits
            ++countRedBits;
            continue;
        }

        // Put the binary value 0 or 1 at the position
        code.emplace_back(s[j] - '0');
        ++j;
    }

    if (countRedBits != redBits) {
        // This condition should never occur as per our previous implementation
        // hence, this code should never be reached
        throw "getHammingCode: Unknown Implementation Error";
    }

    // Evaluating the parity bit at each index which are powers of 2
    for (ll i = 1; i < code.size(); ++i) {
        // If index is not power of 2
        if ((i & (i - 1)) != 0) continue;

        // Counting the number of 1 bits starting from current position, taking
        // i positions, then alternately skipping and taking i positions
        ll countOne = 0;
        for (ll j = i; j < code.size(); j += i) {
            for (ll k = 0; k < i && j < code.size(); ++k, ++j) {
                if (code[j] == 1) ++countOne;
            }
        }

        // If number of 1 bits are even, set parity bit as 0, else 1
        code[i] = (countOne % 2 == 0) ? 0 : 1;
    }

    string ans = "";
    // Join the string to get the final hamming code
    for (ll i = 1; i < code.size(); ++i) {
        ans += ('0' + code[i]);
    }
    return ans;
}

int main(int argc, char const *argv[]) {
    if (argc != EXP_ARGS + 1) {
        // This program requires EXP_ARGS arguments from the command line
        std::cerr << "Expected " << EXP_ARGS << " arguments, but got " << argc - 1 << "\n";
        std::cerr << "Please provide the arguments as follows: " << ARGS_STR << "\n";
        return EXIT_FAILURE;
    }

    // The input message and its length
    string msg = argv[1];
    ll msgLen = msg.length();

    // The input message bits to send with each frame
    ll msgBits = 0;
    try {
        msgBits = stoll(argv[2]);
        if (msgBits <= 0) throw "Number of message bits should be positive!";
    } catch (exception &e) {
        std::cerr << "INVALID ARGUMENTS\n";
        return EXIT_FAILURE;
    }

    // Computing the minimum number of check/redundant bits for the given message bits
    ll redBits = getMinNoRedundantBits(msgBits);

    // The frame size that we want to send
    ll frameSize = msgBits + redBits;

    // Since, server will only accept buffer length of maximum value, we try to restrict the same
    if (frameSize > MAX_BUFF_LEN) {
        std::cerr << "Frame size should not be greater than maximum buffer length which is: " << MAX_BUFF_LEN << "\n";
        return EXIT_FAILURE;
    }

    // The structs required for providing hints and getting the server info
    struct addrinfo hints, *serverInfo;

    // Zero out the struct for providing hints
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET;       // set to AF_INET to use IPv4
    hints.ai_socktype = SOCK_DGRAM;  // for UDP sockets
    hints.ai_flags = AI_PASSIVE;     // use my IP

    ll rv = 0;  // Return value
    // Get the server info using IP and port
    if ((rv = getaddrinfo(IP, PORT, &hints, &serverInfo)) != 0) {
        std::cerr << "client: getaddrinfo: " << gai_strerror(rv) << "\n";
        return EXIT_FAILURE;
    }

    // loop through all the results and make a socket from the first we can
    struct addrinfo *servAddr;
    ll sockfd = 0;  // socket file descriptor
    for (servAddr = serverInfo; servAddr != nullptr; servAddr = servAddr->ai_next) {
        // Create a socket with the same domain, type and protocol as used by the server
        if ((sockfd = socket(servAddr->ai_family, servAddr->ai_socktype, servAddr->ai_protocol)) == -1) {
            perror("client: socket");
            continue;
        }

        // Break as we make the first socket from the results
        break;
    }

    // If while iterating on linked list, we reach the end, then we are unable to create a socket
    if (servAddr == nullptr) {
        std::cerr << "client: Failed to create socket\n";
        return EXIT_FAILURE;
    }

    std::cout << "Input Message: " << msg << "\n"
              << "Length: " << msgLen << "\n\n";

    // The binary representation (8-bit per character) of the input message
    string binMsg = getBinaryMsg(msg);
    ll binMsgLen = binMsg.length();
    std::cout << "Binary representation (8-bit per character): " << binMsg << "\n"
              << "Length: " << binMsgLen << "\n\n";

    // Get the minimum number of padding bits required
    ll padBits = getMinPadBits(binMsgLen, msgBits);

    // Get the padded message
    string paddedMsg = padBinaryString(binMsg, padBits);
    ll paddedMsgLen = paddedMsg.length();
    std::cout << "Number of padding bits required: " << padBits << "\n"
              << "Padded message: " << paddedMsg << "\n"
              << "Length: " << paddedMsgLen << "\n\n";

    // Get the encoded message using 4B/5B encoding
    string preCodedMsg = preCodeMsg(paddedMsg);
    ll preCodedMsgLen = preCodedMsg.length();
    std::cout << "Pre-coded String: " << preCodedMsg << "\n"
              << "Length: " << preCodedMsgLen << "\n\n";

    // Get the final framed message
    string framedMsg = frameMsg(preCodedMsg);
    ll framedMsgLen = framedMsg.length();
    std::cout << "Framed String: " << framedMsg << "\n"
              << "Length: " << framedMsgLen << "\n\n";

    std::cout << "No. of message bits: " << msgBits << "\n"
              << "No. of check bits  : " << redBits << "\n"
              << "No. of frame bits  : " << frameSize << "\n\n";

    // Take the group of `msgBits` from the final framed message
    // and send its hamming code to the server
    for (ll i = 0; i < framedMsgLen; i += msgBits) {
        // The subgroup of message having `m` bits
        string subMsg = framedMsg.substr(i, msgBits);
        // The hamming code of the sub message
        string encodedSubMsg = getHammingCode(subMsg, msgBits, redBits);

        ll encodedSubMsgLen = encodedSubMsg.length();
        if (encodedSubMsgLen != frameSize) {
            // This condition should never occur as per our previous implementation
            // hence, this code should never be reached
            throw "main: getHammingCode: Unexpected Implementation Error";
        }

        ll numBytes = 0;  // Number of bytes sent
        // Send the hamming code to the server using the server information
        if ((numBytes = sendto(sockfd, encodedSubMsg.c_str(), encodedSubMsgLen, 0, servAddr->ai_addr, servAddr->ai_addrlen)) == -1) {
            perror("client: sendto");
            return EXIT_FAILURE;
        }

        // Since, frame size is restricted to be less than the maximum buffer length
        // We are expecting that the whole frame is sent at once
        if (numBytes != encodedSubMsgLen) {
            throw "main: sendto: Unexpected Implementation Error";
        }

        std::cout << "Sent frame to \"" << IP << "\" on Port " << PORT << " with content: " << encodedSubMsg << " of length: " << encodedSubMsgLen << "\n";
    }

    // Release the resources for the server info and the internal linked list
    freeaddrinfo(serverInfo);

    // Close the socket file descriptor
    close(sockfd);

    return EXIT_SUCCESS;
}
