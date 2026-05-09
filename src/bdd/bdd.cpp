#include "bdd.hpp"

BDD::BDD(BDDNode* root, std::vector<std::vector<BDDNode*>> layers) : root(root), layers(std::move(layers)) 
{
}

BDD::~BDD() {
    for(size_t current_layer = 0; current_layer < layers.size(); current_layer++){
        for(BDDNode* node : layers[current_layer]){
            delete node;
        }
    }
}

std::vector<std::vector<int>> BDD::generate_witness_clauses(void) {
    std::vector<std::vector<int>> result;
    for(size_t current_layer = 0; current_layer < layers.size(); current_layer++){
        for(BDDNode* node : layers[current_layer]){
            if(node->get_status() == NodeStatus::INFEASIBLE && !node->get_has_infeasible_parent()){
                std::vector<int> witness_clause = node->get_witness_clause();
                result.push_back(witness_clause);
            }
        }
    }

    std::set<std::vector<int>> s(result.begin(), result.end());
    result.assign(s.begin(), s.end());

    return result;
}

std::string BDD::to_string(void) const {
    std::string result;
    std::unordered_set<const BDDNode*> visited;

    traverse_pre_order(this->root, result, visited);

    result += "total number of nodes: " + std::to_string(visited.size()) + "\n";

    return result;
}

void BDD::traverse_pre_order(const BDDNode* node, std::string& result, std::unordered_set<const BDDNode*>& visited) const {

    if(node == nullptr) return;

    if(visited.count(node) > 0) return;

    visited.insert(node);

    NodeStatus status = node->get_status();
    std::string var_string;
    if(node->get_var() == -1 || status == NodeStatus::SAT || status == NodeStatus::UNSAT){
        var_string = "none";
    }else{
        var_string = std::to_string(node->get_var());
    }
    result += "var: " + var_string + ", ";
    result += "layer: " + std::to_string(node->get_layer()) + ", ";
    result += "state: " + node->get_state() + ", ";
    result += "status: " + status_to_string(status) + " ";

    std::vector<int> witness_clause = node->get_witness_clause();

    if(!witness_clause.empty()){
        result += "witness_clause: " + clause_to_string(witness_clause) + " ";
    }

    result += "\n";

    traverse_pre_order(node->get_low(), result, visited);
    traverse_pre_order(node->get_high(), result, visited);

}

bdd_stats BDD::get_bdd_stats(void) const {
    bdd_stats stats;
    int num_of_default_nodes = 0, num_of_infeasible_nodes = 0, num_of_sat_nodes = 0, num_of_unsat_nodes = 0;
    for(size_t current_layer = 0; current_layer < layers.size(); current_layer++){
        for(BDDNode* node : layers[current_layer]){
            NodeStatus status = node->get_status();
            if(status == NodeStatus::DEFAULT){
                num_of_default_nodes++;
            }else if(status == NodeStatus::INFEASIBLE){
                num_of_infeasible_nodes++;
            }else if(status == NodeStatus::SAT){
                num_of_sat_nodes++;
            }else{
                num_of_unsat_nodes++;
            }
        }
    }

    stats.num_of_nodes = num_of_default_nodes + num_of_infeasible_nodes + num_of_sat_nodes + num_of_unsat_nodes;
    stats.num_of_default_nodes = num_of_default_nodes;
    stats.num_of_infeasible_nodes = num_of_infeasible_nodes;
    stats.num_of_sat_nodes = num_of_sat_nodes;
    stats.num_of_unsat_nodes = num_of_unsat_nodes;
    return stats;
}