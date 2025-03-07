// main.cpp
#include <iostream>
#include "Graph.h"
#include "Scheduler.h"
#include "ILPScheduler.h"

int main(int argc, char* argv[]) {
    if (argc != 6) {
        std::cerr << "Usage: mlrcs -h/-e BLIF_FILE AND_CONSTRAINT OR_CONSTRAINT NOT_CONSTRAINT" << std::endl;
        return 1;
    }

    std::string option = argv[1];
    std::string blifFile = argv[2];
    int andConstraint = std::stoi(argv[3]);
    int orConstraint = std::stoi(argv[4]);
    int notConstraint = std::stoi(argv[5]);

    Graph graph;
    graph.parseBLIF(blifFile);
    
    // graph.printGraph(graph, "graph.dot");

    if (option == "-h") {
        Scheduler scheduler(&graph, andConstraint, orConstraint, notConstraint);
        scheduler.heuristicSchedule();
        scheduler.printSchedule();
    } else if (option == "-e") {
        ILPScheduler ilpScheduler(&graph, andConstraint, orConstraint, notConstraint);
        ilpScheduler.exactSchedule();
        ilpScheduler.printSchedule();
    } else {
        std::cerr << "Invalid option: " << option << std::endl;
        return 1;
    }

    return 0;
}
