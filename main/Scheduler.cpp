// Scheduler.cpp
#include "Scheduler.h"
#include <queue>
#include <iostream>
#include <algorithm>
#include <functional>

Scheduler::Scheduler(Graph* graph, int andLimit, int orLimit, int notLimit)
    : graph(graph), andLimit(andLimit), orLimit(orLimit), notLimit(notLimit), latency(0) {}

int Scheduler::getOperationType(Node* node) const {
    if (node->type == "AND") return 0;
    if (node->type == "OR") return 1;
    if (node->type == "NOT") return 2;
    return -1; // INPUT or OUTPUT
}

void Scheduler::computeNodePriorities() {
    // Use post-order traversal to calculate the priority of nodes
    std::map<Node*, bool> visited;
    std::function<int(Node*)> dfs = [&](Node* node) {
        if (visited[node]) return nodePriority[node];
        visited[node] = true;
        int maxLevel = 0;
        for (Node* succ : node->outputs) {
            int level = dfs(succ);
            if (level > maxLevel) maxLevel = level;
        }
        nodePriority[node] = maxLevel + 1;
        return nodePriority[node];
    };

    for (auto& pair : graph->nodes) {
        Node* node = pair.second;
        if (node->outputs.empty()) { // Endpoint (output node)
            nodePriority[node] = 1;
            visited[node] = true;
        }
    }

    for (auto& pair : graph->nodes) {
        Node* node = pair.second;
        if (!visited[node]) {
            dfs(node);
        }
    }
}

void Scheduler::heuristicSchedule() {
    // Step 1: Calculate the priority of nodes
    computeNodePriorities();

    // Initialize data structures
    std::set<Node*> unscheduledNodes; // Set of unscheduled nodes
    for (auto& pair : graph->nodes) {
        Node* node = pair.second;
        int opType = getOperationType(node);
        if (opType == -1) continue; // INPUT or OUTPUT
        unscheduledNodes.insert(node);
    }

    int currentTime = 1;
    while (!unscheduledNodes.empty()) {
        // Ready nodes (nodes in a ready state)
        std::map<std::string, std::vector<Node*>> readyNodes; // Resource type -> Node list

        for (Node* node : unscheduledNodes) {
            // Check if predecessor nodes have been scheduled
            bool ready = true;
            for (Node* pred : node->inputs) {
                if (unscheduledNodes.find(pred) != unscheduledNodes.end()) {
                    ready = false;
                    break;
                }
            }
            if (ready) {
                std::string opType = node->type;
                readyNodes[opType].push_back(node);
            }
        }

        // Schedule nodes in the current time step
        std::vector<Node*> scheduledThisTime;
        std::map<std::string, int> resourceUsage; // Resource type -> Usage count

        // Initialize resource usage
        resourceUsage["AND"] = 0;
        resourceUsage["OR"] = 0;
        resourceUsage["NOT"] = 0;

        // Schedule nodes for each resource type
        for (auto& resourcePair : readyNodes) {
            std::string opType = resourcePair.first;
            std::vector<Node*>& nodes = resourcePair.second;

            // Sort by priority (higher priority first)
            std::sort(nodes.begin(), nodes.end(), [&](Node* a, Node* b) {
                return nodePriority[a] > nodePriority[b];
            });

            int resourceLimit = 0;
            if (opType == "AND") resourceLimit = andLimit;
            else if (opType == "OR") resourceLimit = orLimit;
            else if (opType == "NOT") resourceLimit = notLimit;

            int availableResource = resourceLimit - resourceUsage[opType];
            int scheduleCount = std::min(static_cast<int>(nodes.size()), availableResource);

            for (int i = 0; i < scheduleCount; ++i) {
                Node* node = nodes[i];
                nodeTime[node] = currentTime;
                scheduledThisTime.push_back(node);
                resourceUsage[opType]++;
                unscheduledNodes.erase(node);
            }
        }

        // Add the scheduling result of the current time step to the schedule
        schedule.push_back(scheduledThisTime);
        currentTime++;
    }

    latency = currentTime - 1;
}

int Scheduler::getNodeTime(Node* node) const {
    auto it = nodeTime.find(node);
    if (it != nodeTime.end()) {
        return it->second;
    } else {
        // If the node is not in nodeTime, return -1 or throw an exception
        return -1;
    }
}

void Scheduler::printSchedule() const {
    std::cout << "Heuristic Scheduling Result" << std::endl;
    for (size_t t = 1; t <= schedule.size(); ++t) {
        std::cout << t << ": ";
        // Initialize resource usage
        std::vector<std::string> andOps, orOps, notOps;
        for (Node* node : schedule[t - 1]) {
            int opType = getOperationType(node);
            if (opType == 0) andOps.push_back(node->name);
            else if (opType == 1) orOps.push_back(node->name);
            else if (opType == 2) notOps.push_back(node->name);
        }
        std::cout << "{";
        for (size_t i = 0; i < andOps.size(); ++i) {
            std::cout << andOps[i];
            if (i != andOps.size() - 1) std::cout << " ";
        }
        std::cout << "} {";
        for (size_t i = 0; i < orOps.size(); ++i) {
            std::cout << orOps[i];
            if (i != orOps.size() - 1) std::cout << " ";
        }
        std::cout << "} {";
        for (size_t i = 0; i < notOps.size(); ++i) {
            std::cout << notOps[i];
            if (i != notOps.size() - 1) std::cout << " ";
        }
        std::cout << "}" << std::endl;
    }
    std::cout << "LATENCY: " << latency << std::endl;
    std::cout << "END" << std::endl;
}
