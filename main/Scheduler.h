// Scheduler.h
#pragma once
#include "Graph.h"
#include <map>
#include <vector>
#include <string>
#include <set>

class Scheduler {
public:
    Scheduler(Graph* graph, int andLimit, int orLimit, int notLimit);
    void heuristicSchedule();
    void printSchedule() const;
    int getLatency() const { return latency; }
    int getNodeTime(Node* node) const;

private:
    Graph* graph;
    int andLimit, orLimit, notLimit;
    int latency;
    std::map<Node*, int> nodeTime;      // Mapping from node to its scheduled time
    std::vector<std::vector<Node*>> schedule; // Schedule per time slot
    std::map<Node*, int> nodePriority;  // Node priority
    void computeNodePriorities();

    int getOperationType(Node* node) const;
};
