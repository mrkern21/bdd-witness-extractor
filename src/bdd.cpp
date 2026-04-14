#include "bdd.hpp"

BDD::BDD(BDDNode* root, std::vector<std::vector<Working_Node>> layers) : root(root), layers(std::move(layers)) 
{
}

BDD::~BDD() {
    std::unordered_set<BDDNode*> visited;
    free_node(root, visited);
}

std::vector<std::vector<int>> BDD::generate_witness_clauses(void) {
    std::vector<std::vector<int>> result = {};
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

    result += "\n";

    traverse_pre_order(node->get_low(), result, visited);
    traverse_pre_order(node->get_high(), result, visited);

}

void BDD::free_node(BDDNode* node, std::unordered_set<BDDNode*>& visited){
    if(node == nullptr) return;
    if(visited.count(node) > 0) return;
    visited.insert(node);

    free_node(node->get_low(), visited);
    free_node(node->get_high(), visited);

    delete node;
}