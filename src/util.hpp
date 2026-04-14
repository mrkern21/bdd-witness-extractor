#ifndef __UTIL_HPP
#define __UTIL_HPP

#include <unordered_map>
#include <vector>

//using forward declaration
class BDDNode;

enum class NodeStatus {
    DEFAULT,
    SAT,
    UNSAT,
    INFEASIBLE
};

inline std::string status_to_string(NodeStatus status){
    switch(status){
        case NodeStatus::DEFAULT:{
            return "DEFAULT";
        }
        case NodeStatus::SAT:{
            return "SAT";
        }
        case NodeStatus::UNSAT:{
            return "UNSAT";
        }
        case NodeStatus::INFEASIBLE:{
            return "INFEASIBLE";
        }
        default: {
            return "UNKNOWN TYPE";
        }
    }
}

struct PartialAssignment {
    BDDNode* node;
    std::unordered_map<int, int> assignment;
};

struct NextState {
    std::vector<std::vector<int>> state;
    NodeStatus status;
};

#endif