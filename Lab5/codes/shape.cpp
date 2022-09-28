#include <fstream>
#include <iomanip>
#include <iostream>

using namespace std;

using ll = long long;
using ld = long double;

int main(int argc, char const *argv[]) {
    if (argc != 3) {
        // This program requires two arguments from the command line
        std::cout << "Expected 2 arguments, but received " << argc - 1 << "\n";
        std::cout << "Please provide the arguments as follows: ./shape <bucketSize> <tokenRate>\n";
        return EXIT_FAILURE;
    }

    // Reading the bucket size and the token rate received as input
    ll bucketSize = 0;
    ld tokenRate = 0;
    try {
        bucketSize = stoll(argv[1]);
        tokenRate = stold(argv[2]);
    } catch (exception &e) {
        std::cout << "INVALID ARGUMENTS\n";
        return EXIT_FAILURE;
    }

    // The arrival and transmission time of the current packet
    ld arrTime = 0, transTime = 0;

    // The packet Id, length and the number of tokens in the bucket
    ll packId = -1, packLen = 0, numTokens = bucketSize;

    // Read from the input till we are able to read the input
    while (cin >> arrTime >> packId >> packLen) {
        // If the arrival time of current packet is greater than the transmission time
        // of the last packet, then there will be some more tokens generated in the bucket
        // upto the maximum bucket size
        if (arrTime > transTime) {
            // Number of tokens generated more between the last transmission time and
            // the current packet arrival time
            numTokens += (arrTime - transTime) * tokenRate;

            // The number of tokens can be maximum of the size of the bucket
            numTokens = min(numTokens, bucketSize);

            // Update the transmission time of the current packet as the arrival time
            transTime = arrTime;
        }

        if (packLen <= numTokens) {
            // If there are enough number of tokens to successfully transmit the whole packet
            // then no change in the transmission time, just reduce that many tokens
            numTokens -= packLen;
        } else {
            // Else, we need to wait more for the extra number of tokens to be generated
            transTime += (packLen - numTokens) / tokenRate;
            // We will be continuously consuming the generated tokens, so after the packet
            // is transmitted, there will be no tokens left
            numTokens = 0;
        }

        // Printing the results in the desired format
        std::cout << std::fixed << std::setprecision(2) << transTime << " " << packId << " " << packLen << "\n";
    }

    return EXIT_SUCCESS;
}
