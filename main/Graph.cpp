// Graph.cpp
#include "Graph.h"
#include <fstream>
#include <sstream>
#include <iostream>

Graph::~Graph() {
    for (auto& pair : nodes) {
        delete pair.second;
    }
}

void Graph::parseBLIF(const std::string& filename) {
    std::ifstream inputFile(filename);
    if (!inputFile.is_open()) {
        std::cerr << "Cannot open BLIF file: " << filename << std::endl;
        exit(1);
    }

    std::string line;
    std::string lastGateName;
    std::string continuation_line;
    int nodeId = 0;

    while (std::getline(inputFile, line)) {
        // Ignore comments after '#'
        size_t commentPos = line.find('#');
        if (commentPos != std::string::npos) {
            line = line.substr(0, commentPos);
        }

        if (!continuation_line.empty()) {
            line = continuation_line + " " + line;
            continuation_line.clear();
        }
        if (!line.empty() && line.back() == '\\') {
            continuation_line = line.substr(0, line.size() - 1);
            continue;
        }

        std::istringstream iss(line);
        std::string token;

        if (line.find(".model") == 0) {
            continue; // Skip .model line
        }
        else if (line.find(".inputs") == 0) {
            // Handle inputs
            while (iss >> token) {
                if (token != ".inputs") {
                    if (nodes.find(token) == nodes.end()) {
                        Node* node = new Node(token, "INPUT", nodeId++);
                        nodes[token] = node;
                        inputs.push_back(node);
                    }
                }
            }
        }
        else if (line.find(".outputs") == 0) {
            // Handle outputs
            while (iss >> token) {
                if (token != ".outputs") {
                    if (nodes.find(token) == nodes.end()) {
                        Node* node = new Node(token, "OUTPUT", nodeId++);
                        nodes[token] = node;
                        outputs.push_back(node);
                    }
                    else {
                        nodes[token]->type = "OUTPUT";
                        outputs.push_back(nodes[token]);
                    }
                }
            }
        }
        else if (line.find(".names") == 0) {
            // Handle gates
            std::vector<std::string> gateTokens;
            while (iss >> token) {
                if (token != ".names") {
                    gateTokens.push_back(token);
                }
            }
            if (!gateTokens.empty()) {
                lastGateName = gateTokens.back(); // The output of the gate
                if (nodes.find(lastGateName) == nodes.end()) {
                    Node* node = new Node(lastGateName, "", nodeId++);
                    nodes[lastGateName] = node;
                }
                for (size_t i = 0; i < gateTokens.size() - 1; ++i) {
                    std::string inputName = gateTokens[i];
                    if (nodes.find(inputName) == nodes.end()) {
                        Node* node = new Node(inputName, "", nodeId++);
                        nodes[inputName] = node;
                    }
                    // Connect input to output
                    nodes[inputName]->outputs.push_back(nodes[lastGateName]);
                    nodes[lastGateName]->inputs.push_back(nodes[inputName]);
                }
            }
        }
        else if (line.find(".end") == 0) {
            break; // End of BLIF file
        }
        else {
            // Determine gate type (AND, OR, NOT)
            if (lastGateName != "") {
                while (iss >> token) {
                    if (token.size() == 1) { // NOT gate
                        nodes[lastGateName]->type = "NOT";
                    }
                    else if (token.find('-') != std::string::npos) { // OR gate
                        nodes[lastGateName]->type = "OR";
                    }
                    else { // AND gate
                        nodes[lastGateName]->type = "AND";
                    }
                    lastGateName = "";
                    break;
                }
            }
        }
    }

    // After parsing, set types for nodes without specified types
    for (auto& pair : nodes) {
        Node* node = pair.second;
        if (node->type == "") {
            if (node->inputs.empty()) {
                node->type = "INPUT";
                inputs.push_back(node);
            }
            else if (node->outputs.empty()) {
                node->type = "OUTPUT";
                outputs.push_back(node);
            }
            else {
                node->type = "WIRE"; // Intermediate wire node
            }
        }
    }

    inputFile.close();
}

// Print graph in DOT format
void Graph::printGraph(const Graph& graph, const std::string& outputFilename) {
    std::ofstream outputFile(outputFilename);
    if (!outputFile.is_open()) {
        std::cerr << "Cannot open output file: " << outputFilename << std::endl;
        return;
    }

    // Write graph in DOT format
    outputFile << "digraph G {" << std::endl;
    for (const auto& pair : graph.nodes) {
        Node* node = pair.second;
        for (Node* output : node->outputs) {
            outputFile << "    \"" << node->name << "\" -> \"" << output->name << "\";" << std::endl;
        }
    }
    outputFile << "}" << std::endl;

    outputFile.close();
    std::cout << "Graph written to " << outputFilename << std::endl;
}