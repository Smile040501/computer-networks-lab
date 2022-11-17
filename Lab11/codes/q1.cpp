// 111901030
// Mayank Singla
// Reference for understanding: https://www.di-mgt.com.au/rsa_alg.html

#include <chrono>
#include <fstream>
#include <iostream>
#include <random>

using namespace std;
using ll = long long;

#define MAX_KEY_LEN 16       // Max key length
#define USER_A_PUB  "A.pub"  // User A public key
#define USER_A_PRI  "A.pri"  // User A private key
#define USER_B_PUB  "B.pub"  // User B public key
#define USER_B_PRI  "B.pri"  // User B private key

// To generate a random number
mt19937 rng(chrono::steady_clock::now().time_since_epoch().count());

// Generates a random number in the given range
ll genRandom(ll low, ll high) {
    return low + (rng() % (high - low + 1));
}

// Generates a n-bit random number
ll nBitRandom(ll n) {
    // Generating a random number b/w [2ⁿ⁻¹ + 1, 2ⁿ - 1]
    ll low = static_cast<ll>(pow(2LL, n - 1)) + 1;
    ll high = static_cast<ll>(pow(2LL, n)) - 1;
    return genRandom(low, high);
}

// Checks if the given number is prime or not in O(sqrt(n))
bool isPrime(ll n) {
    // Check if n is less than 1
    // or n is an even number other than 2
    if (n <= 1 || (n != 2 && ((n & 1) == 0))) return false;

    // Check if n is divisible by any number till sqrt(n)
    for (ll i = 3; i * i <= n; i += 2) {
        if (n % i == 0) return false;
    }

    return true;
}

// Generates a random prime number of given number of bits
ll genPrime(ll numBits) {
    // Generate a random number of given number of bits
    ll p = nBitRandom(numBits);

    // Make the number odd
    p |= 1LL;

    // Set the highest two bits of the number
    p |= (1LL << (numBits - 1));
    p |= (1LL << (numBits - 2));

    // Increment the number by 2 until we find a prime
    while (!isPrime(p)) p += 2;

    return p;
}

// Generate prime pairs p and q for given e and the key length for RSA
pair<ll, ll> genPrimePairs(ll e, ll keyLen) {
    ll p = 0, q = 0;
    do {
        // Generate a random prime number of bit length k/2
        // until its module with e is not 1
        do {
            p = genPrime(keyLen / 2);
        } while (p % e == 1);

        // Generate a random prime number of bit length k - k/2
        // until its module with e is not 1
        do {
            q = genPrime(keyLen - (keyLen / 2));
        } while (q % e == 1);

        // In the extreme rare case, if p and q are equal
        // generate the numbers again
    } while (p == q);

    // Keep p bigger than q
    if (p < q) swap(p, q);

    return make_pair(p, q);
}

// Triplet class to store the solution for equation ax + by = gcd(a, b)
class Triplet {
   public:
    ll x, y, gcd;
};

// Solve the equation ax + by = gcd(a, b)
Triplet extendedEuclid(ll a, ll b) {
    if (b == 0) {
        // Base case
        Triplet ans;
        ans.gcd = a;
        ans.x = 1;
        ans.y = 0;  // Any integer
        return ans;
    }

    /** There also exists an equation
     * b.x1 + (a % b)y1 = gcd(b, a%b)
     * Replacing: (a % b) = (a - b * floor(a / b))
     * Comparing both equation as RHS is same:
     * x = y1
     * y = x1 - floor(a/b) * y1
     */
    Triplet smallAns = extendedEuclid(b, a % b);
    Triplet ans;
    ans.gcd = smallAns.gcd;
    ans.x = smallAns.y;
    ans.y = smallAns.x - ((a / b) * smallAns.y);
    return ans;
}

// Extended Euclid algorithm to solve ax + by = gcd(a, b)
// Returns the gcd(a, b)
ll extendedEuclid(ll a, ll b, ll &x, ll &y) {
    Triplet ans = extendedEuclid(a, b);
    x = ans.x;
    y = ans.y;
    return ans.gcd;
}

/** Function to find the multiplicative modulo inverse of a and m
 * (a.x) mod m = 1
 * (a.x - 1) = my       (y is any integer)
 * (a.x + m(-y)) = 1
 * Equivalent to solving: ax + my = 1
 */
pair<bool, ll> modInv(ll a, ll m) {
    ll x = 0, y = 0;
    ll gcd = extendedEuclid(a, m, x, y);

    // Solution only exists if gcd(a, m) is 1
    if (gcd != 1) return make_pair(false, 0);

    // As x can be negative
    x = (x % m + m) % m;
    return make_pair(true, x);
}

// Function to generate public-private key pairs using RSA
pair<pair<ll, ll>, ll> genPublicPrivateKeys(ll e = -1) {
    // Take a random value for public exponent if not given
    e = (e == -1) ? vector<ll>{3, 5, 17, 257, 65537}[genRandom(0, 4)] : e;

    // The maximum key length
    ll keyLen = MAX_KEY_LEN;

    // Generate prime pairs p and q such that gcd(e, phi(n)) = 1
    pair<ll, ll> primePair = genPrimePairs(e, keyLen);
    ll p = primePair.first, q = primePair.second;

    ll n = p * q;

    // Calculate phi
    ll phi = (p - 1) * (q - 1);

    // Calculate the multiplicative modulo inverse of e and phi
    auto inv = modInv(e, phi);

    if (!inv.first) {
        std::cerr << "GCD of e and phi is not 1\n";
        exit(EXIT_FAILURE);
    }

    // The private exponent
    ll d = inv.second;

    return make_pair(make_pair(n, e), d);
}

// Creates the public-private RSA keys and stores them in the given public and private files
void createRSAKeys(string pubFileName, string priFileName) {
    // Generate keys
    auto keys = genPublicPrivateKeys();
    ll n = keys.first.first, e = keys.first.second, d = keys.second;

    // Open the public and private files
    std::ofstream pubFile(pubFileName, std::ios::out);
    std::ofstream priFile(priFileName, std::ios::out);

    // Check if not opened
    if (!pubFile || !priFile) {
        std::cerr << "Unable to open the file\n";
        exit(EXIT_FAILURE);
    }

    // Store the public and the private keys
    pubFile << e << " " << n;
    priFile << d << " " << n;

    // Close the files
    pubFile.close();
    priFile.close();
}

int main() {
    // Create public-private keys for User A
    createRSAKeys(USER_A_PUB, USER_A_PRI);

    // Create public-private keys for User B
    createRSAKeys(USER_B_PUB, USER_B_PRI);

    return EXIT_SUCCESS;
}
