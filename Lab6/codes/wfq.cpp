// 111901030
// Mayank Singla

#include <iomanip>
#include <iostream>
#include <map>
#include <queue>
#include <vector>

// The minimum expected number of input arguments
#define MIN_EXP_ARGS 1

// The argument string to execute the program
#define ARGS_STR "./wfq <serviceRate> <wt-q1> <wt-q2> <wt-q3> ..."

using namespace std;

using ll = long long;
using ld = long double;

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

    // The weight of the queue
    ld weight;

    // Underlying queue of packets
    queue<Packet> packetQ;

    // Constructors
    Queue()
        : queueId{-1}, weight{1}, packetQ{queue<Packet>()} {}

    Queue(ll queueId, ld weight, queue<Packet> packetQ)
        : queueId{queueId}, weight{weight}, packetQ{packetQ} {}

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

void readInput(const vector<ld> &qWeights, ll &numPackets, vector<Queue> &queues) {
    // The arrival time of the current packet
    ld arrTime = 0;

    // The packet Id and length
    ll packId = -1, queueId = -1, packLen = 0;

    // All the different queues and packets coming into them
    map<ll, Queue> ques;

    // Number of packets in the input
    numPackets = 0;

    while (cin >> arrTime >> packId >> queueId >> packLen) {
        // Increment the number of packets
        ++numPackets;

        // If this queue Id is not present in the map, initialize it
        if (ques.find(queueId) == ques.end()) {
            // Initially setting the queue weight as 1
            ques[queueId] = Queue(queueId, 1, queue<Packet>());
        }

        // Insert the packet into the queue ID for the queue
        ques[queueId].push(Packet(arrTime, packId, queueId, packLen));
    }

    if (qWeights.size() != ques.size()) {
        throw "Number of distinct queues is not same as the number of queues for which weights are given";
    }

    // Clearing the input queues
    queues.clear();

    // Insert Queue into the vector
    for (const auto &p : ques) {
        ll idx = queues.size();
        queues.emplace_back(p.second);
        // Set the queue weight to what is received from the input
        queues[idx].weight = qWeights[idx];
    }
}

int main(int argc, char const *argv[]) {
    if (argc < MIN_EXP_ARGS + 1) {
        // This program requires minimum of  MIN_EXP_ARGS arguments from the command line
        std::cout << "Expected minimum of " << MIN_EXP_ARGS << " arguments, but got " << argc - 1 << "\n";
        std::cout << "Please provide the arguments as follows: " << ARGS_STR << "\n";
        return EXIT_FAILURE;
    }

    // Reading the service rate received as input
    ld serviceRate = 0;
    vector<ld> qWeights;
    try {
        serviceRate = stold(argv[1]);
        for (ll i = 2; i < argc; ++i) {
            qWeights.emplace_back(stold(argv[i]));
        }
    } catch (exception &e) {
        std::cout << "INVALID ARGUMENTS\n";
        return EXIT_FAILURE;
    }

    // The total number of packets
    ll numPackets = 0;

    // All the queues from the input
    vector<Queue> queues;

    // Read the input
    readInput(qWeights, numPackets, queues);

    // Custom comparator for the pair (transTime, Packet) to use with priority queue
    // If transmission time is same, the packet with the lower packet ID is given the preference
    auto cmp = [](const pair<ld, Packet> &x, const pair<ld, Packet> &y) {
        if (x.first == y.first) {
            return x.second.packId > y.second.packId;
        }
        return x.first > y.first;
    };

    // Min-priority queue for the pair (transTime, Packet) using the above custom comparator
    priority_queue<pair<ld, Packet>, vector<pair<ld, Packet>>, decltype(cmp)> pq(cmp);

    // Calculating the virtual finish time of all the packets in the queues
    for (auto &q : queues) {
        // The previous finish time of the last packet in the queue,
        // also serve as the finish time for the current packet after update
        ld finishTime = 0;

        // While the queue is not empty
        while (!q.empty()) {
            // Take the front packet
            Packet p = q.front();
            // Remove the packet from the queue
            q.pop();
            // Calculate the finish time for the current packet
            // Fᵢ = max(Aᵢ, Fᵢ₋₁) + (Lᵢ / W)
            // We will serve the packet as per the weight of the queue
            finishTime = max(p.arrTime, finishTime) + (p.packLen / q.weight);
            // Push the pair (finishTime, Packet) into the priority queue
            pq.emplace(make_pair(finishTime, p));
        }
    }

    // The previous transmission time of the last packet in the queue,
    // also serve as the transmission time for the current packet after update
    ld transTime = 0;

    // Take the packets in decreasing order of their transmission times and print the results
    while (!pq.empty()) {
        auto p = pq.top();
        pq.pop();
        // Update the transmission time
        transTime = max(p.second.arrTime, transTime) + (p.second.packLen / serviceRate);
        // Printing the results in the desired format
        std::cout << std::fixed << std::setprecision(2) << transTime << " " << p.second.packId << " " << p.second.queueId << "\n";
    }

    return EXIT_SUCCESS;
}
