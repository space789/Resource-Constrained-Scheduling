// Graph.h
#pragma once
#include <string>
#include <map>
#include <vector>
#include "Node.h"

class Graph {
public:
    std::map<std::string, Node*> nodes; // Node name to Node pointer
    std::vector<Node*> inputs;          // Input nodes
    std::vector<Node*> outputs;         // Output nodes

    ~Graph();
    void parseBLIF(const std::string& filename);
    void printGraph(const Graph& graph, const std::string& outputFilename);
};
