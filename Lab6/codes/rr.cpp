// 111901030
// Mayank Singla

#include <iomanip>
#include <iostream>
#include <map>
#include <queue>
#include <vector>

// The expected number of input arguments
#define EXP_ARGS 1

// The argument string to execute the program
#define ARGS_STR "./rr <serviceRate>"

using namespace std;

using ll = long long;
using ld = float;

// Packet class to represent a packet
class Packet {
   public:
    // The arrival time of the current packet
    ld arrTime;

    // The packet Id and length
    ll packId, queueId, packLen;

    // Constructor
    Packet(ld arrTime, ll packId, ll queueId, ll packLen)
        : arrTime{arrTime}, packId{packId}, queueId{queueId}, packLen{packLen} {}
};

// Queue class to represent a queue
class Queue {
   public:
    // The queue Id
    ll queueId;

    // Underlying queue of packets
    queue<Packet> packetQ;

    // Constructors
    Queue()
        : queueId{-1}, packetQ{queue<Packet>()} {}

    Queue(ll queueId, queue<Packet> packetQ)
        : queueId{queueId}, packetQ{packetQ} {}

    // Whether the queue is empty or not
    bool empty() const {
        return packetQ.empty();
    }

    // Get the front packet from the queue
    const Packet &front() {
        return packetQ.front();
    }

    // Push a packet into the queue
    void push(const Packet &p) {
        packetQ.emplace(p);
    }

    // Pop the front packet from the queue
    void pop() {
        packetQ.pop();
    }
};

void readInput(ll &startQueueIdx, ll &numPackets, vector<Queue> &queues) {
    // The arrival time of the current packet
    ld arrTime = 0;

    // The packet Id and length, and the id of the first queue
    ll packId = -1, queueId = -1, packLen = 0, startQueueId = 0;

    // If it's the first packet
    bool firstPacket = true;

    // All the different queues and packets coming into them
    map<ll, Queue> ques;

    // Number of packets in the input
    numPackets = 0;

    while (cin >> arrTime >> packId >> queueId >> packLen) {
        // Increment the number of packets
        ++numPackets;

        // If it's the first packet, store its queue id and mark firstPacket as false
        if (firstPacket) {
            startQueueId = queueId;
            firstPacket = false;
        }

        // If this queue Id is not present in the map, initialize it
        if (ques.find(queueId) == ques.end()) {
            ques[queueId] = Queue(queueId, queue<Packet>());
        }

        // Insert the packet into the queue ID for the queue
        ques[queueId].push(Packet(arrTime, packId, queueId, packLen));
    }

    // Clearing the input queues
    queues.clear();

    // Insert Queue into the vector
    for (const auto &p : ques) {
        if (p.first == startQueueId) {
            // If it is the first queue, populate its index with the size of the current vector of queues
            startQueueIdx = queues.size();
        }
        queues.emplace_back(p.second);
    }
}

// Function to find the index of the queue having the packet with the minimum arrival time
// In case of tie, the queue with larger queue Id is given the preference
ll getMinArrTimePackQueueIdx(vector<Queue> &queues) {
    ld minArrTime = 0;
    ll idx = -1, minQueueId = 0;
    for (ll i = 0; i < queues.size(); ++i) {
        if (queues[i].empty()) {
            // Skip if the queue is empty
            continue;
        }

        ld arrTime = queues[i].front().arrTime;
        ll queueId = queues[i].front().queueId;

        // The logic for finding the packet with minimum arrival time
        // And in case of tie, the queue with larger queue Id is given the preference
        if ((idx == -1) || (arrTime < minArrTime) || (arrTime == minArrTime && queueId > minQueueId)) {
            idx = i;
            minArrTime = arrTime;
            minQueueId = queueId;
        }
    }
    return idx;
}

int main(int argc, char const *argv[]) {
    if (argc != EXP_ARGS + 1) {
        // This program requires EXP_ARGS arguments from the command line
        std::cout << "Expected " << EXP_ARGS << " arguments, but got " << argc - 1 << "\n";
        std::cout << "Please provide the arguments as follows: " << ARGS_STR << "\n";
        return EXIT_FAILURE;
    }

    // Reading the service rate received as input
    ld serviceRate = 0;
    try {
        serviceRate = stold(argv[1]);
    } catch (exception &e) {
        std::cout << "INVALID ARGUMENTS\n";
        return EXIT_FAILURE;
    }

    // The current queue index on which we are processing and the total number of packets
    ll currQueueIdx = 0, numPackets = 0;

    // All the queues and their packets in them
    vector<Queue> queues;

    // Read the input
    readInput(currQueueIdx, numPackets, queues);

    // The transmission time of the last packet
    // also serves as the transmission time for the current packet after update
    ld transTime = 0;

    // The number of packets transmitted, number of queues skipped and the total number of queues
    ll numPacksTrans = 0, queuesSkipped = 0, numQueues = queues.size();

    // If it is the first packet to process
    bool firstPacket = true;

    // Loop till all the packets are not transmitted
    while (numPacksTrans != numPackets) {
        // If it is the first packet to process
        if (firstPacket) {
            // Initialize the transmission time with the arrival time of the first packet
            transTime = queues[currQueueIdx].front().arrTime;
            // Mark first Packet as false
            firstPacket = false;
        }

        // If we have skipped all the queues for once, then we need to
        // jump to the queue having the packet with the minimum arrival time
        if (queuesSkipped == numQueues) {
            // Find the index of the queue having the packet with the minimum arrival time
            currQueueIdx = getMinArrTimePackQueueIdx(queues);
            // Update the transmission time as the arrival time for this packet
            transTime = queues[currQueueIdx].front().arrTime;
            // Reset number of queues skipped to 0
            queuesSkipped = 0;
        }

        // If the current queue is empty
        if (queues[currQueueIdx].empty()) {
            // Increment the index to move to the next queue in cyclic order
            currQueueIdx = (currQueueIdx + 1) % numQueues;
            // Increment the number of queues skipped
            ++queuesSkipped;
            continue;
        }

        // Get the front packet at the current queue
        Packet currPack = queues[currQueueIdx].front();

        // If its arrival time is greater than the transmission time, then skip the queue
        if (currPack.arrTime > transTime) {
            // Increment the index to move to the next queue in cyclic order
            currQueueIdx = (currQueueIdx + 1) % numQueues;
            // Increment the number of queues skipped
            ++queuesSkipped;
            continue;
        }

        // Update the transmission time for the current packet
        transTime += (currPack.packLen / serviceRate);
        // Increment the number of packets transmitted
        ++numPacksTrans;
        // Pop the packet from the corresponding queue
        queues[currQueueIdx].pop();
        // Reset the number of queues skipped to 0
        queuesSkipped = 0;
        // Increment the index to move to the next queue in cyclic order
        currQueueIdx = (currQueueIdx + 1) % numQueues;

        // Printing the results in the desired format
        std::cout << std::fixed << std::setprecision(2) << transTime << " " << currPack.packId << "\n";
    }

    return EXIT_SUCCESS;
}
