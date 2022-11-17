// 111901030
// Mayank Singla

#include <fstream>
#include <iostream>
#include <vector>

using namespace std;
using ll = long long;

#define USER_A_PUB "A.pub"        // User A private key
#define USER_B_PRI "B.pri"        // User B public key
#define MSG_FILE   "message.txt"  // The message to encrypt
#define SEC_FILE   "secret.txt"   // The secret file to store the signed and encrypted text
#define HASH_PRIME 97             // Prime used for hashing

// Calculates (x^y) % m using binary exponentiation
ll pw(ll x, ll y, ll m) {
    // Trivial cases
    if (x == 0 || x == 1 || y == 1) return x;

    // Take modulo before start
    x %= m;

    // The final result
    ll res = 1;

    // Whenever we encounter a set bit in y, we multiple result with x
    // Taking multiplicative modulo always
    while (y > 0) {
        if ((y & 1) != 0) {
            res = res * x % m;  // res = res*x
        }
        x = x * x % m;  // x = x*x
        y >>= 1;
    }

    return res;
}

// Calculates the message digest for a given character
// Same message digest function as used by the sender
ll msgDigest(char c) {
    return static_cast<ll>(HASH_PRIME) * static_cast<ll>(c);
}

// The reverse hash function
char reverseDigest(ll m) {
    return static_cast<char>(m / HASH_PRIME);
}

// Reads the secret message from the secret file and decrypts it using the given private key
vector<ll> decrypt(string priFileName, string secFileName) {
    // Open the private key file
    std::ifstream priFile(priFileName, std::ios::in);
    if (!priFile) {
        std::cerr << "Unable to open the file: " << priFileName << '\n';
        exit(EXIT_FAILURE);
    }
    // Read d and n
    ll d = 0, n = 0;
    priFile >> d >> n;
    // Close the private key file
    priFile.close();

    // Decrypt each encrypted message in the secret file
    vector<ll> ans;
    std::ifstream secFile(secFileName, std::ios::in);
    if (!secFile) {
        std::cerr << "Unable to open the file: " << secFileName << '\n';
        exit(EXIT_FAILURE);
    }
    while (!secFile.eof()) {
        ll c = 0;
        secFile >> c;
        // Decrypt message, P = C^d mod n
        ans.emplace_back(pw(c, d, n));
    }
    // Close the secret file
    secFile.close();

    return ans;
}

// Unsigns the given decrypted messages using the given public file
vector<ll> unsign(string pubFileName, const vector<ll> &cs) {
    // Open the public key file
    std::ifstream pubFile(pubFileName, std::ios::in);
    if (!pubFile) {
        std::cerr << "Unable to open the file: " << pubFileName << '\n';
        exit(EXIT_FAILURE);
    }
    // Read e and n
    ll e = 0, n = 0;
    pubFile >> e >> n;
    // Close the public key file
    pubFile.close();

    // Unsign each decrypted message
    vector<ll> ans;
    for (const auto &c : cs) {
        ans.emplace_back(pw(c, e, n));
    }

    return ans;
}

// Validates the received message file from the extracted message digests from the unsigned messages
bool validateMsgDigest(string msgFileName, const vector<ll> &mds) {
    // Open the received message file
    std::ifstream msgFile(msgFileName, std::ios::in);
    if (!msgFile) {
        std::cerr << "Unable to open the file: " << msgFileName << '\n';
        exit(EXIT_FAILURE);
    }

    char p = '\0';
    ll idx = 0;
    // Read each character from the received message file
    // and validate if the message digest of both the
    // received file and extracted message digest are same
    while (msgFile.get(p)) {
        if (idx >= mds.size()) return false;
        if (msgDigest(p) != mds[idx]) {
            return false;
        }
        ++idx;
    }
    // Close the received message file
    msgFile.close();

    return true;
}

// Decrypts and unsigns the received secret message using the given public and private keys
// and validates it with the received message file
void decryptAndUnsign(string priFileName, string msgFileName, string pubFileName, string secFileName) {
    // Open the private key file
    std::ifstream priFile(priFileName, std::ios::in);
    if (!priFile) {
        std::cerr << "Unable to open the file: " << priFileName << '\n';
        exit(EXIT_FAILURE);
    }
    // Read d and n
    ll db = 0, nb = 0;
    priFile >> db >> nb;
    // Close the private key file
    priFile.close();

    // Open the public key file
    std::ifstream pubFile(pubFileName, std::ios::in);
    if (!pubFile) {
        std::cerr << "Unable to open the file: " << pubFileName << '\n';
        exit(EXIT_FAILURE);
    }
    // Read e and n
    ll ea = 0, na = 0;
    pubFile >> ea >> na;
    // Close the public key file
    pubFile.close();

    // Get the unsigned message digests
    vector<ll> mds;
    // If n for user B is less than n for user A
    if (nb < na) {
        // First decrypt using the public key of user A
        // then unsign using the private key of user B
        mds = unsign(priFileName, decrypt(pubFileName, secFileName));
    } else {
        // First decrypt using the private key of user B
        // then unsign using the public key of user A
        mds = unsign(pubFileName, decrypt(priFileName, secFileName));
    }

    // Validate the received message digests with the received message file
    if (!validateMsgDigest(msgFileName, mds)) {
        std::cout << "Message not verified\n";
        return;
    }

    // If valid, reverse the message digest to get the original message
    string msg = "";
    for (const auto &m : mds) {
        msg += reverseDigest(m);
    }
    std::cout << "Received Msg: " << msg << '\n';
}

int main() {
    // Decrypt the received secret using the private key of B
    // Unsign the decrypted message using the public key of A message
    // and validate it with the received message file
    decryptAndUnsign(USER_B_PRI, MSG_FILE, USER_A_PUB, SEC_FILE);
    return EXIT_SUCCESS;
}
