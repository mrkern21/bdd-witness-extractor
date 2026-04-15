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

inline std::string clause_to_string(std::vector<int> clause){
    std::string result = "[";

    for(size_t i = 0; i < clause.size(); i++){
        result += std::to_string(clause[i]) + " ";
    }

    result.pop_back();
    result += "]";

    return result;
}

struct NextState {
    std::vector<std::vector<int>> state;
    std::vector<int> witness_clause;
    NodeStatus status;
};

#endif