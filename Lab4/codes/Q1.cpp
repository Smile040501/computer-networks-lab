#include <fstream>
#include <iomanip>
#include <iostream>
#include <map>
#include <vector>

using namespace std;
using ll = long long;

// Max edge weight given in the question
#define MAX_EDGE_WEIGHT 1e5

// INFINITY
const ll INF = 1e18;

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
ll buildGraph(ll numNodes, const vector<Edge> &edges, map<string, ll> &nti, map<ll, string> &itn, string sourceNode, vector<vector<pair<ll, ll>>> &gr) {
    const ll sNode = 0;  // Give source node as integer 0
    nti[sourceNode] = sNode;
    itn[sNode] = sourceNode;

    // Initialize the adjacency list for each node in the graph
    gr.assign(numNodes, vector<pair<ll, ll>>());

    ll num = 1;  // The integer assigned to each string name of the node

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
    }

    // Return the integer assigned to the source node
    return sNode;
}

// Performs the Dijkstra algorithm on the given graph from the source node
// fills the distance and the parent vectors from the source node
void dijkstra(ll numNodes, ll sourceNode, vector<ll> &distance, vector<ll> &parent, const vector<vector<pair<ll, ll>>> &gr) {
    // Keep track of which nodes have been visited so far. Initially all the nodes are not visited
    vector<bool> visited(numNodes, false);

    // Initialize the distance of all the nodes from the source node as INFINITY
    distance.assign(numNodes, INF);
    // Initialize the parent of all the nodes as -1 in their shortest path from the source node
    parent.assign(numNodes, -1);

    distance[sourceNode] = 0;  // Distance of the source node from itself is zero

    // Iterate over all the nodes
    for (ll i = 0; i < numNodes; ++i) {
        // Find the node having the minimum distance among all the unvisited nodes
        int u = -1;
        for (int j = 0; j < numNodes; ++j) {
            if (!visited[j] && (u == -1 || distance[j] < distance[u])) {
                u = j;
            }
        }

        // It means that all the remaining unvisited nodes are unreachable from the source node
        if (distance[u] == INF) break;

        visited[u] = true;  // Mark this node as visited

        // Iterate over all the edges of the current node
        for (const auto &e : gr[u]) {
            // The neighbor node and the edge weight
            ll v = e.first, w = e.second;
            // If the neighbor node can be reached from the source node in less distance
            // when using the current node, then update the distance of the neighbor node
            // and marks its parent as the current node on the least cost path
            if (distance[u] + w < distance[v]) {
                distance[v] = distance[u] + w;
                parent[v] = u;
            }
        }
    }
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

// Pretty prints the results obtained from the Dijkstra algorithm
// Output the least cost path from source node to every node in the network along with the path price
void printResults(ll sNode, const vector<ll> &distance, const vector<ll> &parent, const map<ll, string> &itn) {
    std::cout << "Distance of Nodes from the source node: " << (itn.find(sNode))->second << "\n\n";
    prettyPrint("Node", 15);
    std::cout << "  |  ";
    prettyPrint("Path Price", 15);
    std::cout << "  |  ";
    std::cout << "Shortest Path\n";
    prettyPrint("\n", 75, std::right, '=');

    // Iterating all the nodes in the network
    for (ll i = 0; i < distance.size(); ++i) {
        // Convert the node from integer naming to original string name
        auto currNode = itn.find(i);

        // If the node is not present in the map, then it does not have any edge in the network
        // So, its name is unkown
        prettyPrint(currNode != itn.end() ? currNode->second : "-", 15);
        std::cout << "  |  ";

        // If node is unreachable from the source node
        if (distance[i] == INF) {
            prettyPrint("INF", 15);
            std::cout << "  |  ";
            std::cout << "-";
        } else {
            prettyPrint(distance[i], 15);
            std::cout << "  |  ";

            // Extracting the path by going from node to its parent till we reach the source node
            vector<string> path;
            for (ll v = i; v != -1; v = parent[v]) {
                path.emplace_back((itn.find(v))->second);
            }
            // Printing the least cost path
            for (ll j = path.size() - 1; j >= 0; --j) {
                std::cout << path[j];
                if (j != 0) std::cout << " -> ";
            }
        }
        std::cout << "\n";
    }
}

int main(int argc, char const *argv[]) {
    // This program requires three arguments from the command line
    if (argc != 4) {
        std::cout << "Expected 3 arguments, but received " << argc - 1 << "\n";
        std::cout << "Please provide the arguments as follows: ./<prog_name.out> <numNodes> <sourceNode> <fileName>";
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

    string sourceNode = argv[2];  // The source node
    string fileName = argv[3];    // The filename containing edges information

    // Reading the input edges from the file
    vector<Edge> edges;
    readEdges(fileName, edges);

    // Constructing the graph from the edges, and
    // constructing the map and reverse map from string name to integer name for the nodes
    map<string, ll> nodeToInt;
    map<ll, string> intToNode;
    vector<vector<pair<ll, ll>>> graph;
    ll sNode = buildGraph(numNodes, edges, nodeToInt, intToNode, sourceNode, graph);

    // Executing the dijkstra algorithm
    vector<ll> distance, parent;
    dijkstra(numNodes, sNode, distance, parent, graph);

    // Printing the results obtained
    printResults(sNode, distance, parent, intToNode);

    return EXIT_SUCCESS;
}
