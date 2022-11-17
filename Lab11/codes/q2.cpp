// 111901030
// Mayank Singla

#include <fstream>
#include <iostream>
#include <vector>

using namespace std;
using ll = long long;

#define USER_A_PRI "A.pri"        // User A private key
#define USER_B_PUB "B.pub"        // User B public key
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
ll msgDigest(char c) {
    return static_cast<ll>(HASH_PRIME) * static_cast<ll>(c);
}

// Reads a text message from a file and signs it based on the given private key
vector<ll> sign(string priFileName, string msgFileName) {
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

    // For each character in the message file
    // sign the message digest for each character
    std::ifstream msgFile(msgFileName, std::ios::in);
    if (!msgFile) {
        std::cerr << "Unable to open the file: " << msgFileName << '\n';
        exit(EXIT_FAILURE);
    }
    char p = '\0';
    vector<ll> ans;
    while (msgFile.get(p)) {
        // Signed message, C = P^d mod n
        ans.emplace_back(pw(msgDigest(p), d, n));
    }
    // Close the message file
    msgFile.close();
    return ans;
}

// Encrypts the given signed messages with the given public key
// and stores it in the given secret file as space separated
void encrypt(string pubFileName, string secFileName, const vector<ll> &ps) {
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

    // Open the secret file to write encrypted message
    std::ofstream secFile(secFileName, std::ios::out);
    if (!secFile) {
        std::cerr << "Unable to open the file: " << secFileName << '\n';
        exit(EXIT_FAILURE);
    }
    for (ll i = 0; i < ps.size(); ++i) {
        // Encrypted message: C = P^e mod n
        secFile << pw(ps[i], e, n);
        if (i != ps.size() - 1) secFile << " ";
    }
    secFile.close();
}

// Signs and Encrypts the message given the public and private keys to use
// and stores the encrypted message to the given secret file
void signAndEncrypt(string priFileName, string msgFileName, string pubFileName, string secFileName) {
    // Open the private key file
    std::ifstream priFile(priFileName, std::ios::in);
    if (!priFile) {
        std::cerr << "Unable to open the file: " << priFileName << '\n';
        exit(EXIT_FAILURE);
    }
    // Read d and n
    ll da = 0, na = 0;
    priFile >> da >> na;
    // Close the private key file
    priFile.close();

    // Open the public key file
    std::ifstream pubFile(pubFileName, std::ios::in);
    if (!pubFile) {
        std::cerr << "Unable to open the file: " << pubFileName << '\n';
        exit(EXIT_FAILURE);
    }
    // Read e and n
    ll eb = 0, nb = 0;
    pubFile >> eb >> nb;
    // Close the public key file
    pubFile.close();

    // If n for user B is less than n for user A
    if (nb < na) {
        // First sign using public key of user B
        // then encrypt using private key of user A
        encrypt(priFileName, secFileName, sign(pubFileName, msgFileName));
        return;
    }

    // First sign using private key of user A
    // then encrypt using public key of user B
    // and store the message in secret.txt
    encrypt(pubFileName, secFileName, sign(priFileName, msgFileName));
}

int main() {
    // Sign the message using private key of user A
    // and encrypt the signed message using public key of B
    signAndEncrypt(USER_A_PRI, MSG_FILE, USER_B_PUB, SEC_FILE);
    return EXIT_SUCCESS;
}
