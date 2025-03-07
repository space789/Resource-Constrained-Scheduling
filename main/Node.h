// Node.h
#pragma once
#include <string>
#include <vector>

class Node {
public:
    std::string name;
    std::string type; // "AND", "OR", "NOT", "INPUT", "OUTPUT"
    int id; // Unique identifier
    std::vector<Node*> inputs;
    std::vector<Node*> outputs;

    Node(const std::string& name, const std::string& type, int id);
};
