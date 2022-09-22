#include <algorithm>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <map>
#include <random>
#include <vector>

using namespace std;
using ll = long long;

// Max edge weight given in the question
#define MAX_EDGE_WEIGHT 1e5

// Max number of iterations to consider for convergence when there are no updates to the distance vectors
#define MAX_NO_UPDATE_CONV 100

// INFINITY
const ll INF = 1e18;

// Overloading stream insertion operator for generic pairs
template <typename T1, typename T2>
ostream &operator<<(ostream &os, const pair<T1, T2> &p) {
    // Initially constructing a stringstream object so that we can pass the whole string
    // together to the ostream object to conserve pretty printing
    stringstream s;
    s << '{' << p.first << ", " << p.second << '}';
    os << s.str();
    return os;
}

// Represents an edge of the network
class Edge {
   public:
    // The vertices `u` and `v` of an edge and the weight `w` of that edge
    string u, v;
    ll w;

    // Constructor
    Edge(string u, string v, ll w) : u{u}, v{v}, w{w} {}
};

// Takes a string as input and convert it to a long long integer
// If the string contains non-digit characters, it will throw error
ll stringToLong(const string &s) {
    bool isNeg = false;
    if (s[0] == '-') isNeg = true;  // whether the number is negative or not

    ll ans = 0;
    for (ll i = 0 + isNeg; i < s.length(); ++i) {
        if (!isdigit(s[i])) throw exception();
        // construct the number
        ans *= 10;
        ans += (s[i] - '0');
    }
    return isNeg ? -ans : ans;
}

// Reads input edges from the given filename
// and fills it inside the input vector of edges
void readEdges(string fileName, vector<Edge> &edges) {
    // The `ifstream` object to read an input file
    ifstream inFile{fileName, ios::in};

    // If the file was not able to open, exit with failure status
    if (!inFile) {
        std::cout << "File '" << fileName << "' could not be opened!\n";
        exit(EXIT_FAILURE);
    }

    // The vertices and weight of an edge
    string u, v, w;
    ll wl;
    // Read from the file till we are able to read the input
    while (inFile >> u >> v >> w) {
        try {
            // Try converting weight to long long integer
            wl = stringToLong(w);

            // For this question weight should be a positive integer
            // less than 10000
            if (wl <= 0 || wl >= MAX_EDGE_WEIGHT) throw exception();
        } catch (exception &e) {
            std::cout << "The weight of an edge between two nodes should be a positive integer less than 10,000, but got '" << w << "'\n";
            inFile.close();  // Close the file
            exit(EXIT_FAILURE);
        }

        // Create and push the edge into the vector
        edges.push_back(Edge(u, v, wl));
    }

    // Close the input file
    inFile.close();
}

// Builds a graph from the given set of edges
// Maps the string name of each node to normalized integer numbers starting from 0
// and maintains a reverse map of the above. Returns the integer assigned to the source node
// also initializes the distance vector of each input node
void buildGraph(ll numNodes, const vector<Edge> &edges, map<string, ll> &nti, map<ll, string> &itn, vector<vector<pair<ll, ll>>> &gr, vector<vector<pair<ll, ll>>> &dv) {
    ll num = 0;  // The integer assigned to each string name of the node

    // Initialize the adjacency list for each node in the graph
    gr.assign(numNodes, vector<pair<ll, ll>>());

    // Initialize the distance vector for each node with distance as INFINITY and neighbor as -1
    dv.assign(numNodes, vector<pair<ll, ll>>(numNodes, make_pair(INF, -1)));

    // Loop over all the edges
    for (const Edge &e : edges) {
        ll ui, vi;                  // The integers for both the vertices of the edge
        auto uItr = nti.find(e.u);  // Try finding the string name in the map
        if (uItr != nti.end()) {
            // If it already exists, take that integer
            ui = uItr->second;
        } else {
            // Else assign it a new integer and update the map
            ui = num++;
            nti[e.u] = ui;
            itn[ui] = e.u;
        }

        auto vItr = nti.find(e.v);  // Try finding the string name in the map
        if (vItr != nti.end()) {
            // If it already exists, take that integer
            vi = vItr->second;
        } else {
            // Else assign it a new integer and update the map
            vi = num++;
            nti[e.v] = vi;
            itn[vi] = e.v;
        }

        // The provided input number of nodes and number of distinct nodes found in the
        // input file should be same
        if (num > numNodes) {
            std::cout << "Number of distinct nodes in the input file is greater than the given number of nodes\n";
            exit(EXIT_FAILURE);
        }

        // Push the edge into the adjacency list of both the vertices
        gr[ui].emplace_back(make_pair(vi, e.w));
        gr[vi].emplace_back(make_pair(ui, e.w));

        // Initializing the distance vector of the nodes
        dv[ui][ui] = make_pair(0, ui);
        dv[vi][vi] = make_pair(0, vi);
        dv[ui][vi] = make_pair(e.w, vi);
        dv[vi][ui] = make_pair(e.w, ui);
    }
}

// Executes the distance vector routing algorithm on the input graph till convergence
void distVecRouting(ll numNodes, const vector<vector<pair<ll, ll>>> &gr, vector<vector<pair<ll, ll>>> &dv) {
    // Initialize nodes numbering from 0 to n-1
    vector<ll> nodes(numNodes);
    iota(nodes.begin(), nodes.end(), 0);

    // Create a random number generator
    random_device rd;
    mt19937 generator(rd());

    // Number of iterations and number of iterations when there is no update
    int numIter = 0, noUpdate = 0;

    // Executing the algorithm
    while (true) {
        bool isUpdated = false;  // Whether there update occurs in this iteration

        // Randomly shuffle all the nodes
        shuffle(nodes.begin(), nodes.end(), generator);

        // Iterate over all the nodes
        for (auto &u : nodes) {
            // Take the neighbors of the current node `u`
            auto neighbors = gr[u];

            // make sure all neighbors have an equal probability of picking
            // range is inclusive, so 0 to m-1
            uniform_int_distribution<size_t> distribution(0, neighbors.size() - 1);

            // Generate a random index to pick any random neighbor
            size_t randIndex = distribution(generator);

            // Take the randomly picked neighbor
            auto &n = neighbors[randIndex];
            // The neighbor vertex and weight of the edge u-v
            ll v = n.first, dist_u_v = n.second;

            // Iterate over all the nodes to update the distance vector of the current node `u`
            // using the distance vector of the neighbor node `v`
            for (ll i = 0; i < dv[v].size(); ++i) {
                // The current distance between the nodes u-i and v-i
                ll dist_u_i = dv[u][i].first, dist_v_i = dv[v][i].first;

                // If the node `i` is unreachable from `v`, skip
                if (dist_v_i == INF) continue;

                // If the distance between node v-i plus u-v is less than the distance
                // between the nodes u-i, then update the distance vector of `u` for the node `i`
                // via the neighbor `v`
                if (dist_v_i + dist_u_v < dist_u_i) {
                    dv[u][i] = make_pair(dist_v_i + dist_u_v, v);
                    isUpdated = true;  // An update occurred
                    noUpdate = 0;      // Reset this variable
                }
            }
        }

        ++numIter;                   // Increment the number of iterations
        if (!isUpdated) ++noUpdate;  // Increment if there is no update

        // If we reached max count when there are no updates, we conclude that convergence has occurred
        if (noUpdate == MAX_NO_UPDATE_CONV) {
            numIter -= (noUpdate - 1);  // Subtract the extra iterations added from the actual convergence
            break;
        }
    }

    std::cout << "Number of iterations for convergence: " << numIter << "\n\n";
}

// Generic function to pretty print any input with defined width, alignment and fill character
template <typename T>
void prettyPrint(const T &res, ll width, std::ios_base &(*positioning)(std::ios_base &) = std::left, char fillChar = ' ') {
    ios init(NULL);
    // copying the default `cout` formatting
    init.copyfmt(std::cout);
    // pretty printing the input
    std::cout << setfill(fillChar) << setw(width) << positioning << res;
    // resetting the default `cout` formatting
    std::cout.copyfmt(init);
}

// Pretty prints the results obtained from the Distance Vector Routing algorithm
// Output the local routing table at every node after convergence has occurred
void printResults(const map<ll, string> &itn, const vector<vector<pair<ll, ll>>> &dv) {
    prettyPrint("", 5);
    for (ll i = 0; i < dv.size(); ++i) {
        std::cout << "  |  ";
        prettyPrint(itn.find(i)->second, 10);
    }
    std::cout << "\n";
    prettyPrint("\n", 5 + (15 * dv.size()), std::right, '=');

    // Iterating all the nodes in the network
    for (int i = 0; i < dv.size(); ++i) {
        // Convert the node from integer naming to original string name
        auto currNode = itn.find(i);

        // If the node is not present in the map, then it does not have any edge in the network
        // So, its name is unkown
        prettyPrint(currNode != itn.end() ? currNode->second : "-", 5);

        // Printing the distance vector of the current node
        for (auto &p : dv[i]) {
            std::cout << "  |  ";
            if (p.first == INF) {
                // If node is unreachable from the current node
                prettyPrint(make_pair("INF", "-"), 10);
            } else {
                // Print the distance of the node from the current node and
                // the neighbor that should be taken to reach that node from the current node
                prettyPrint(make_pair(p.first, itn.find(p.second)->second), 10);
            }
        }
        std::cout << "\n";
    }
}

int main(int argc, char const *argv[]) {
    // This program requires two arguments from the command line
    if (argc != 3) {
        std::cout << "Expected 2 arguments, but received " << argc - 1 << "\n";
        std::cout << "Please provide the arguments as follows: ./<prog_name.out> <numNodes> <fileName>";
        return EXIT_FAILURE;
    }

    // Getting the number of nodes in the network
    ll numNodes;
    try {
        numNodes = stringToLong(argv[1]);
        if (numNodes <= 0) throw exception();
    } catch (exception &e) {
        std::cout << "Number of nodes should be provided as a positive integer, got '" << argv[1] << "'\n";
        return EXIT_FAILURE;
    }

    string fileName = argv[2];  // The filename containing edges information

    // Reading the input edges from the file
    vector<Edge> edges;
    readEdges(fileName, edges);

    // Constructing the graph from the edges, and
    // constructing the map and reverse map from string name to integer name for the nodes
    // and initializing the distance vector for all the nodes
    map<string, ll> nodeToInt;
    map<ll, string> intToNode;
    vector<vector<pair<ll, ll>>> graph;
    vector<vector<pair<ll, ll>>> distVec;
    buildGraph(numNodes, edges, nodeToInt, intToNode, graph, distVec);

    // Executing the Distance Vector Routing algorithm
    distVecRouting(numNodes, graph, distVec);

    // Printing the results obtained
    printResults(intToNode, distVec);

    return EXIT_SUCCESS;
}
