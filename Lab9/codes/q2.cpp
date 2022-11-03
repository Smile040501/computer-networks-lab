// 111901030
// Mayank Singla

#include <bitset>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>

using namespace std;
namespace fs = std::filesystem;  // Filesystem namespace

using ll = long long;
using ull = unsigned long long;
using checksum_frag = short;  // Datatype for number of bits taken at a time to compute the checksum - 16 bits (2 bytes) -

#define DIR_PATH             "../ipfrags"  // Directory path to the IP fragments
#define NUM_IP_ADDR_BYTES    4             // Number of bytes in the IP address contained in IPv4 packet
#define PACK_DATA_LEN        1024          // Given Max length of the data in the IP packet
#define PACK_VERSION_LEN     4             // Number of bits in the version field of the IP packet
#define PACK_HEADER_LEN      4             // Number of bits in the header field of the IP packet
#define PACK_FLAGS_LEN       3             // Number of bits in the flags field of the IP packet
#define PACK_FRAG_OFFSET_LEN 13            // Number of bits in fragmentation offset field of the IP packet

// Function to convert a value (char or int families) to their binary representation
template <typename T>
string getBinary(T val) {
    // Using bitset to convert to binary number
    bitset<sizeof(T) * 8> b(static_cast<ull>(val));
    return b.to_string();
}

// Function to convert a binary string to decimal representation
ull binaryToInt(const string& s) {
    // Using bitset to convert to decimal number
    bitset<sizeof(int) * 8> b(s);
    return b.to_ullong();
}

// Function to convert a 32-bit IP address into dotted notation form
string getIP(const unsigned char* addr) {
    string result = "";
    // Reading 8 binary bits (1 byte) at a time and converting to its decimal notation form
    for (ll i = 0; i < NUM_IP_ADDR_BYTES; ++i) {
        // char is of 1 byte, so take its binary representation
        string boct = getBinary(addr[i]);

        // Append the decimal notation to the result
        result += to_string(binaryToInt(boct));

        // If it is not the last set of 8 bits, append a dot
        if (i != 3) result += ".";
    }
    return result;
}

// Struct to represent an IPv4 Datagram Packet
using IPPacket = struct IPPacket_t {
    unsigned char v_hl;                      // Version(4 bits) + Header_Length(4 bits)
    unsigned char dscp_ecn;                  // Type_of_Service(8 bits)
    unsigned short totalLen;                 // Datagram_Length(16 bits)
    unsigned short id;                       // Identifier(16 bits)
    unsigned short flags_frag_offset;        // Flags(3 bits) + Fragmentation_Offset(13 bits)
    unsigned char ttl;                       // Time_to_Live(8 bits)
    unsigned char proto;                     // Upper_Layer_Protocol(8 bits)
    unsigned short checksum;                 // Header_Checksum(16 bits)
    unsigned char sAddr[NUM_IP_ADDR_BYTES];  // Source_IP_Address(32 bits)
    unsigned char dAddr[NUM_IP_ADDR_BYTES];  // Destination_IP_Address(32 bits)
    unsigned int o1;                         // Header_Options(32 bits)
    unsigned int o2;                         // Header_Options(32 bits)
    unsigned char data[PACK_DATA_LEN];       // Data

    // Method to get the version field of the IP packet
    ull getVersion() const noexcept {
        // Take the very first four bits (from MSB) from the `v_hl` field
        string bv_hl = getBinary(this->v_hl);
        string bv = bv_hl.substr(0, PACK_VERSION_LEN);
        return binaryToInt(bv);
    }

    // Method to get the header length field of the IP packet
    ull getHeaderLen() const noexcept {
        // Take the last four bits (from MSB) from the `v_hl` field
        string bv_hl = getBinary(this->v_hl);
        string bhl = bv_hl.substr(PACK_VERSION_LEN, PACK_HEADER_LEN);
        return binaryToInt(bhl);
    }

    // Method to get the Type-of-Service field of the IP packet
    ull getTOS() const noexcept {
        string btos = getBinary(this->dscp_ecn);
        return binaryToInt(btos);
    }

    // Method to get the Total Datagram Length field of the IP packet
    ull getDatagramLen() const noexcept {
        return static_cast<ull>(this->totalLen);
    }

    // Method to get the Identifier field of the IP packet
    unsigned short getID() const noexcept {
        return this->id;
    }

    // Method to get all flags (Reserved Flag, Don't Fragment Flag, More Fragment Flag) of the IP packet
    pair<ull, pair<ull, ull>> getFlags() const noexcept {
        // Take the very first 3 bits (from MSB) of the `flags_frag_offset` field
        string bflags_frag_offset = getBinary(this->flags_frag_offset);
        string bflags = bflags_frag_offset.substr(0, PACK_FLAGS_LEN);
        // Reserved Flag       = Bit0
        // Don't Fragment Flag = Bit1
        // More Fragment Flag  = Bit2
        return make_pair(binaryToInt(string(1, bflags[2])), make_pair(binaryToInt(string(1, bflags[1])), binaryToInt(string(1, bflags[0]))));
    }

    // Method to get the fragmentation offset field of the IP packet
    ull getFragOffset() const noexcept {
        // Take the last 13 bits (from MSB) of the `flags_frag_offset` field
        string bflags_frag_offset = getBinary(this->flags_frag_offset);
        string bfrag_offset = bflags_frag_offset.substr(PACK_FLAGS_LEN, PACK_FRAG_OFFSET_LEN);
        return binaryToInt(bfrag_offset);
    }

    // Method to get the Time-to-Live field of the IP packet
    ull getTTL() const noexcept {
        string bttl = getBinary(this->ttl);
        return binaryToInt(bttl);
    }

    // Method to get the Upper-Layer-Protocol field of the IP packet
    ull getProtocol() const noexcept {
        string bproto = getBinary(this->proto);
        return binaryToInt(bproto);
    }

    // Method to get the Header checksum field of the IP packet in binary representation
    string getChecksum() const noexcept {
        return getBinary(this->checksum);
    }

    // Method to get the Source IP address field of the IP packet in dotted notation form
    string getSourceIP() const noexcept {
        return getIP(this->sAddr);
    }

    // Method to get the Destination IP address field of the IP packet in dotted notation form
    string getDestinationIP() const noexcept {
        return getIP(this->dAddr);
    }

    // Method to get the Options field of the IP packet
    pair<unsigned int, unsigned int> getOptions() const noexcept {
        return make_pair(this->o1, this->o2);
    }

    // Method to get the Data Length of the IP packet
    ull getDataLen() const noexcept {
        return this->getDatagramLen() - (this->getHeaderLen() * 4);
    }

    // Method to get the Data present in the IP packet
    string getData() const noexcept {
        ull dataLen = this->getDataLen();
        return string(reinterpret_cast<const char*>(this->data), dataLen);
    }
};

// Number of bits taken at a time to compute the checksum 2 * 8 = 16 bits
#define CHECKSUM_FRAG_SIZE sizeof(checksum_frag) * 8

// Number of set of 16-bits (2 bytes) taken to compute the checksum: (1052 - 1024) / 2 = 14
#define NUM_CHECKSUM_FRAG (sizeof(IPPacket) - PACK_DATA_LEN) / sizeof(checksum_frag)

// Function to add two binary bits given the input carry
bool addBinaryBits(bool a, bool b, bool& carry) {
    bool sum = a ^ b ^ carry;                // Sum = A ⊕ B ⊕ Cᵢₙ
    carry = (a && b) || (carry && (a ^ b));  // Cₒᵤₜ = AB + Cᵢₙ (A ⊕ B)
    return sum;
}

// Function to add two binary numbers based on 1's compliment addition
bitset<CHECKSUM_FRAG_SIZE> addBinaryNums(const bitset<CHECKSUM_FRAG_SIZE>& n, const bitset<CHECKSUM_FRAG_SIZE>& m) {
    bool carry = false;                    // The carry bit
    bitset<CHECKSUM_FRAG_SIZE> result(0);  // The final result
    // Add each bit at a time based on binary addition
    for (size_t i = 0; i < static_cast<size_t>(CHECKSUM_FRAG_SIZE); ++i) {
        result[i] = addBinaryBits(n[i], m[i], carry);
    }

    // If there is no carry at the end, then return the result
    if (!carry) {
        return result;
    }

    // If there is a carry left, then add 1 to the result sum
    return addBinaryNums(result, bitset<CHECKSUM_FRAG_SIZE>(carry));
}

// Function to compute the header checksum of the IP packet
bitset<CHECKSUM_FRAG_SIZE> computeChecksum(const IPPacket* packet) {
    // The sum of 2 bytes taken at a time using 1's compliment addition
    bitset<CHECKSUM_FRAG_SIZE> sum(0);
    const checksum_frag* p = reinterpret_cast<const checksum_frag*>(packet);
    for (ll i = 0; i < static_cast<ll>(NUM_CHECKSUM_FRAG); ++i) {
        string binary = getBinary(*p);
        // Add the 2 bytes to the resulting sum
        sum = addBinaryNums(sum, bitset<CHECKSUM_FRAG_SIZE>(binary));
        p = p + 1;
    }
    // Take the 1's compliment of the final sum to get the checksum
    return sum.flip();
}

// Function to check if the header checksum of the IP packet is valid or not
bool validateChecksum(const IPPacket* packet) {
    // Header checksum of the received IP packet should come out to be zero
    return computeChecksum(packet).to_ullong() == 0;
}

// Function to print the IP packet details
void printPacketDetails(const IPPacket* packet) {
    // The flag bits in the IP packet
    pair<ull, pair<ull, ull>> flags = packet->getFlags();

    // The options field in the IP packet
    pair<unsigned int, unsigned int> options = packet->getOptions();

    std::cout << std::left << setw(30) << "Version"
              << " : " << packet->getVersion() << '\n'
              << std::left << setw(30) << "Header Length (bytes)"
              << " : " << packet->getHeaderLen() << '\n'
              << std::left << setw(30) << "Type of Service"
              << " : " << packet->getTOS() << '\n'
              << std::left << setw(30) << "Datagram Length (bytes)"
              << " : " << packet->getDatagramLen() << '\n'
              << std::left << setw(30) << "16-bit Identifier"
              << " : " << packet->getID() << '\n'
              << std::left << setw(30) << "Flags"
              << " : "
              << "Reserved Bit(" << flags.first << "), Don't Fragment Bit(" << flags.second.first << "), More Fragment Bit(" << flags.second.second << ")\n"
              << std::left << setw(30) << "13-bit Fragmentation Offset"
              << " : " << packet->getFragOffset() << '\n'
              << std::left << setw(30) << "Time-to-Live"
              << " : " << packet->getTTL() << '\n'
              << std::left << setw(30) << "Upper-Layer Protocol"
              << " : " << packet->getProtocol() << '\n'
              << std::left << setw(30) << "Header Checksum"
              << " : " << packet->getChecksum() << '\n'
              << std::left << setw(30) << "32-bit Source IP Address"
              << " : " << packet->getSourceIP() << '\n'
              << std::left << setw(30) << "32-bit Destination IP Address"
              << " : " << packet->getDestinationIP() << '\n'
              << std::left << setw(30) << "Options"
              << " : " << options.first << ", " << options.second << '\n';
}

int main() {
    // Number of legitimate packets found
    ll countLegitimate = 0;

    // Path to the directory to read the IP packets
    fs::path ipfrags(DIR_PATH);

    // Iterate over each file in the directory
    for (auto const& dirEntry : fs::directory_iterator(ipfrags)) {
        // Open the file in binary mode
        ifstream inFile(dirEntry.path(), std::ios::binary);

        // If file not opened
        if (!inFile) {
            std::cerr << "File " << dirEntry.path() << " couldn't be opened\n";
            exit(EXIT_FAILURE);
        }

        // Create an IP packet struct and populate it with the binary data from the file
        IPPacket* packet = new IPPacket();
        inFile.read(reinterpret_cast<char*>(packet), sizeof(*packet));

        // If the header checksum of the IP packet is valid
        if (validateChecksum(packet)) {
            // Increment the number of legitimate packets found
            ++countLegitimate;

            // Print packet details
            std::cout << std::left << std::setw(30) << "Packet"
                      << " : " << dirEntry.path().filename() << '\n';
            printPacketDetails(packet);
            std::cout << '\n';
        }

        // Release the space for the packet
        delete packet;

        // Close the file
        inFile.close();
    }

    std::cout << "Number of legitimate packets found: " << countLegitimate << '\n';

    return EXIT_SUCCESS;
}
