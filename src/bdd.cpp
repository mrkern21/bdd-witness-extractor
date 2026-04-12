#include "bdd.hpp"

BDD::BDD(BDDNode* root, std::vector<BDDNode*>& unsat_nodes) : unsat_nodes(unsat_nodes) {
    this->root = root;
}

BDD::~BDD() {
    delete this->root;
}

std::vector<std::vector<int>> BDD::generate_witness_clauses() {
    std::vector<std::vector<int>> result = {};
    return result;
}

void BDD::traverse_pre_order(const BDDNode* node, std::string& result, std::unordered_set<const BDDNode*>& visited) const {

    if(node == nullptr) return;

    if(visited.count(node) > 0) return;

    visited.insert(node);

    result += "var: " + std::to_string(node->get_var()) + " ";
    result += "layer: " + std::to_string(node->get_layer());
    result += "\n";

    traverse_pre_order(node->get_low(), result, visited);
    traverse_pre_order(node->get_high(), result, visited);

}

std::string BDD::to_string() const {
    std::string result;
    std::unordered_set<const BDDNode*> visited;

    traverse_pre_order(this->root, result, visited);

    return result;
}