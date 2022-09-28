#include <deque>
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
        std::cout << "Please provide the arguments as follows: ./shape <bufferSize> <outDataRate>\n";
        return EXIT_FAILURE;
    }

    // Reading the buffer size and the output data rate
    ll bufferSize = 0;
    ld outDataRate = 0;
    try {
        bufferSize = stoll(argv[1]);
        outDataRate = stold(argv[2]);
    } catch (exception &e) {
        std::cout << "INVALID ARGUMENTS\n";
        return EXIT_FAILURE;
    }

    // The arrival and transmission time of the current packet
    ld arrTime = 0, transTime = 0;

    // The packet Id and length
    ll packId = -1, packLen = 0;

    // Simulate the FIFO queue
    deque<pair<ld, pair<ll, ll>>> fifo;

    // Read from the input till we are able to read the input
    while (cin >> arrTime >> packId >> packLen) {
        // The current remaining capacity of the FIFO queue
        ll capacity = bufferSize;

        // Remove the packets from the FIFO queue which are finished transmitting
        // till the arrival time of the new packet
        // and compute the remaining capacity left of the FIFO queue
        auto it = fifo.begin();
        bool stopped = false;  // whether the packets in the queue are stopped transmitting
        while (it != fifo.end()) {
            // The arrival time, packet id and length of the packet in the queue
            ld thisArrTime = (*it).first;
            ll thisPackId = (*it).second.first, thisPackLen = (*it).second.second;

            if (stopped) {
                // If they are stopped transmitting
                // simply reduce the capacity by the packet length
                capacity -= thisPackLen;
                ++it;
                continue;
            }

            if (thisArrTime > transTime) {
                // If the arrival time is greater, update the transmission time
                // as for this packet the transmission will start from its arrival time
                transTime = thisArrTime;
            }

            // Length of the packet that can be sent till the current arrival time
            ll packetSent = (arrTime - transTime) * outDataRate;

            if (packetSent < thisPackLen) {
                // If this packet can't be transmitted fully, then mark the stop as true
                stopped = true;
                // We will still reduce the capacity by the whole packet length
                // as the HOL packet is held in the buffer until it is transmitted completely
                capacity -= thisPackLen;
                ++it;
            } else {
                // Else the packet could be transmitted,
                // and update its transmission time by the time it takes the packet to transmit
                transTime += thisPackLen / outDataRate;
                // Printing the results
                std::cout << std::fixed << std::setprecision(2) << transTime << " " << thisPackId << " " << thisPackLen << "\n";
                // Pop out this packet from the queue
                fifo.pop_front();
                // Update the iterator
                it = fifo.begin();
            }
        }

        if (capacity >= packLen) {
            // If the remaining capacity in the queue is greater than the packet length
            // then push the packet into the queue
            fifo.push_back(make_pair(arrTime, make_pair(packId, packLen)));
        }
    }

    // Process the remaining packets in the queue
    while (!fifo.empty()) {
        // Pop out the packet from the queue
        auto p = fifo.front();
        fifo.pop_front();

        // The arrival time, packet id and length of the packet in the queue
        ld thisArrTime = p.first;
        ll thisPackId = p.second.first, thisPackLen = p.second.second;

        if (thisArrTime > transTime) {
            // If the arrival time is greater, update the transmission time
            // as for this packet the transmission will start from its arrival time
            transTime = thisArrTime;
        }

        // Update its transmission time by the time it takes the packet to transmit
        transTime += thisPackLen / outDataRate;
        std::cout << std::fixed << std::setprecision(2) << transTime << " " << thisPackId << " " << thisPackLen << "\n";
    }

    return EXIT_SUCCESS;
}
