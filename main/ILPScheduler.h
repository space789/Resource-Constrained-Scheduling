// ILPScheduler.h
#pragma once
#include "Graph.h"
#include <gurobi_c++.h>
#include <map>
#include <vector>
#include <string>

class ILPScheduler {
public:
    ILPScheduler(Graph* graph, int andLimit, int orLimit, int notLimit);
    void exactSchedule();
    void printSchedule() const;

private:
    Graph* graph;
    int andLimit, orLimit, notLimit;
    int latency;
    std::map<Node*, int> nodeTime;
    std::vector<std::vector<Node*>> schedule;

    int getOperationType(Node* node) const;
};
