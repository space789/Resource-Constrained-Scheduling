#include "ILPScheduler.h"
#include "Scheduler.h"
#include <thread>
#include <iostream>

ILPScheduler::ILPScheduler(Graph* graph, int andLimit, int orLimit, int notLimit)
    : graph(graph), andLimit(andLimit), orLimit(orLimit), notLimit(notLimit), latency(0) {}

int ILPScheduler::getOperationType(Node* node) const {
    if (node->type == "AND") return 0;
    if (node->type == "OR") return 1;
    if (node->type == "NOT") return 2;
    return -1; // INPUT or OUTPUT
}

void ILPScheduler::exactSchedule() {
    try {
        // Determine number of threads
        int numThreads = std::thread::hardware_concurrency();
        if (numThreads == 0) {
            numThreads = 4; // Default to 4 if unable to get hardware concurrency
        }

        GRBEnv env = GRBEnv(true);
        env.set("LogFile", ""); // Disable Gurobi log file
        env.set("OutputFlag", "0"); // Suppress Gurobi log output
        env.start();

        GRBModel model = GRBModel(env);

        // Set Gurobi parameters
        model.set(GRB_IntParam_Threads, numThreads);
        model.set(GRB_DoubleParam_TimeLimit, 900); // Set time limit
        model.set(GRB_IntParam_Presolve, 1);
        model.set(GRB_IntParam_Cuts, 2);

        // Heuristic scheduling
        Scheduler heuristicScheduler(graph, andLimit, orLimit, notLimit);
        heuristicScheduler.heuristicSchedule();
        int heuristicLatency = heuristicScheduler.getLatency();

        // Need to schedule operation nodes (AND, OR, NOT)
        std::vector<Node*> operationNodes;
        for (auto& pair : graph->nodes) {
            Node* node = pair.second;
            int opType = getOperationType(node);
            if (opType != -1) {
                operationNodes.push_back(node);
            }
        }

        int maxLatency = heuristicLatency;

        // Variables: x_i_t = 1 if node i starts at time t
        std::map<Node*, std::vector<GRBVar>> x;
        for (Node* node : operationNodes) {
            std::vector<GRBVar> varList;
            for (int t = 1; t <= maxLatency; ++t) {
                varList.push_back(model.addVar(0, 1, 0, GRB_BINARY, "x_" + node->name + "_" + std::to_string(t)));
            }
            x[node] = varList;
        }

        // Latency variable
        GRBVar latencyVar = model.addVar(1, maxLatency, 0, GRB_INTEGER, "latency");

        // Constraints:

        // 1. Each operation must be scheduled exactly once
        for (Node* node : operationNodes) {
            GRBLinExpr expr = 0;
            for (int t = 1; t <= maxLatency; ++t) {
                expr += x[node][t - 1];
            }
            model.addConstr(expr == 1, "sched_once_" + node->name);
        }

        // 2. Dependency Constraints
        for (Node* node : operationNodes) {
            for (Node* pred : node->inputs) {
                int opTypePred = getOperationType(pred);
                if (opTypePred == -1) {
                    // pred is INPUT node
                    // Node cannot start before time 1
                    // No need to add constraint since x[node][t] is defined for t >= 1
                    continue;
                } else {
                    // For all possible times
                    for (int t = 1; t <= maxLatency; ++t) {
                        GRBLinExpr expr = 0;
                        for (int tp = 1; tp <= t - 1; ++tp) {
                            expr += x[pred][tp - 1];
                        }
                        model.addConstr(x[node][t - 1] <= expr, "dep_" + pred->name + "_" + node->name + "_t" + std::to_string(t));
                    }
                }
            }
        }

        // 3. Resource Constraints
        for (int t = 1; t <= maxLatency; ++t) {
            GRBLinExpr andExpr = 0, orExpr = 0, notExpr = 0;
            for (Node* node : operationNodes) {
                int opType = getOperationType(node);
                if (opType == 0) {
                    andExpr += x[node][t - 1];
                } else if (opType == 1) {
                    orExpr += x[node][t - 1];
                } else if (opType == 2) {
                    notExpr += x[node][t - 1];
                }
            }
            model.addConstr(andExpr <= andLimit, "and_limit_t" + std::to_string(t));
            model.addConstr(orExpr <= orLimit, "or_limit_t" + std::to_string(t));
            model.addConstr(notExpr <= notLimit, "not_limit_t" + std::to_string(t));
        }

        // 4. Latency Constraints
        for (Node* node : operationNodes) {
            GRBLinExpr sum = 0;
            for (int t = 1; t <= maxLatency; ++t) {
                sum += t * x[node][t - 1];
            }
            model.addConstr(latencyVar >= sum, "latency_constr_" + node->name);
        }

        // For OUTPUT nodes
        for (Node* node : graph->outputs) {
            for (Node* pred : node->inputs) {
                int opTypePred = getOperationType(pred);
                if (opTypePred == -1) {
                    // pred is INPUT node
                    model.addConstr(latencyVar >= 1, "output_dep_input_" + pred->name + "_" + node->name);
                } else {
                    GRBLinExpr sum = 0;
                    for (int t = 1; t <= maxLatency; ++t) {
                        sum += t * x[pred][t - 1];
                    }
                    model.addConstr(latencyVar >= sum, "output_dep_" + pred->name + "_" + node->name);
                }
            }
        }

        // Objective: Minimize latency
        model.setObjective(GRBLinExpr(latencyVar), GRB_MINIMIZE);

        // Set initial solution from heuristic scheduler
        for (Node* node : operationNodes) {
            int t = heuristicScheduler.getNodeTime(node);
            if (t != -1) {
                x[node][t - 1].set(GRB_DoubleAttr_Start, 1.0);
            }
        }
        latencyVar.set(GRB_DoubleAttr_Start, heuristicLatency);

        // Optimize model
        model.optimize();

        if (model.get(GRB_IntAttr_Status) == GRB_INFEASIBLE) {
            std::cout << "Model is infeasible" << std::endl;
            model.computeIIS();
            model.write("infeasible.ilp");
            return;
        }

        // Extract the schedule
        latency = static_cast<int>(latencyVar.get(GRB_DoubleAttr_X) + 0.5);
        schedule.resize(latency);
        for (Node* node : operationNodes) {
            for (int t = 1; t <= latency; ++t) {
                if (x[node][t - 1].get(GRB_DoubleAttr_X) > 0.5) {
                    nodeTime[node] = t;
                    schedule[t - 1].push_back(node);
                    break; // Node is scheduled exactly once
                }
            }
        }

    } catch (GRBException& e) {
        std::cerr << "Gurobi Error code = " << e.getErrorCode() << std::endl;
        std::cerr << e.getMessage() << std::endl;
    } catch (...) {
        std::cerr << "Unknown Gurobi error during ILP scheduling." << std::endl;
    }
}

void ILPScheduler::printSchedule() const {
    std::cout << "ILP-based Scheduling Result" << std::endl;
    for (size_t t = 1; t <= schedule.size(); ++t) {
        std::cout << t << ": ";
        // Initialize resource usage counts
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
    std::cout << "LATENCY: " << schedule.size() << std::endl;
    std::cout << "END" << std::endl;
}
