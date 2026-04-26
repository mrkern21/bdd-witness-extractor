#ifndef __UTIL_HPP
#define __UTIL_HPP

#include <unordered_map>
#include <vector>

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

inline std::string clause_to_string(std::vector<int>& clause){
    std::string result = "[";

    for(size_t i = 0; i < clause.size(); i++){
        result += std::to_string(clause[i]) + " ";
    }

    result.pop_back();
    result += "]";

    return result;
}

inline void print_witness_clauses(std::vector<std::vector<int>>& clauses){
    for(std::vector<int>& clause : clauses){
        std::cout << clause_to_string(clause) << std::endl;
    }
}

inline void print_sat_result(int result){
    std::cout << "result: ";
    if(result == 10){
        std::cout << "SAT" << std::endl;
    }else if(result == 20){
        std::cout << "UNSAT" << std::endl;
    }else{
        std::cout << "UNKNOWN" << std::endl;
    }
}

struct NextState {
    std::vector<std::vector<int>> state;
    std::vector<int> witness_clause;
    NodeStatus status;
};

struct bdd_stats {
    int num_of_nodes;
    int num_of_sat_nodes;
    int num_of_unsat_nodes;
    int num_of_infeasible_nodes;
    int num_of_default_nodes;
};

#endif