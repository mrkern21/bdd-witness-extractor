#include "bdd.hpp"

BDD::BDD(BDDNode* root, std::vector<BDDNode*> unsat_nodes) : root(root), unsat_nodes(std::move(unsat_nodes)) 
{
}

BDD::~BDD() {
    std::unordered_set<BDDNode*> visited;
    free_node(root, visited);
}


//TODO: Should this be removed?
BDD::BDD(BDD&& other) noexcept
    : root(other.root), unsat_nodes(std::move(other.unsat_nodes))
{
    other.root = nullptr;
}

BDD& BDD::operator=(BDD&& other) noexcept
{
    if (this != &other)
    {
        std::unordered_set<BDDNode*> visited;
        free_node(root, visited);

        root = other.root;
        unsat_nodes = std::move(other.unsat_nodes);

        other.root = nullptr;
    }
    return *this;
}

std::vector<std::vector<int>> BDD::generate_witness_clauses() {
    std::vector<std::vector<int>> result = {};
    return result;
}

void BDD::traverse_pre_order(const BDDNode* node, std::string& result, std::unordered_set<const BDDNode*>& visited) const {

    if(node == nullptr) return;

    if(visited.count(node) > 0) return;

    visited.insert(node);

    std::string var_string;
    if(node->get_var() != -1){
        var_string = std::to_string(node->get_var());
    }else{
        var_string = "none";
    }
    result += "var: " + var_string + ", ";
    result += "layer: " + std::to_string(node->get_layer()) + ", ";
    result += "state: " + node->get_state() + ", ";
    result += "status: " + status_to_string(node->get_status()) + " ";

    result += "\n";

    traverse_pre_order(node->get_low(), result, visited);
    traverse_pre_order(node->get_high(), result, visited);

}

std::string BDD::to_string() const {
    std::string result;
    std::unordered_set<const BDDNode*> visited;

    traverse_pre_order(this->root, result, visited);

    result += "total number of nodes: " + std::to_string(visited.size()) + "\n";

    return result;
}

void BDD::free_node(BDDNode* node, std::unordered_set<BDDNode*>& visited){
    if(node == nullptr) return;
    if(visited.count(node) > 0) return;
    visited.insert(node);

    free_node(node->get_low(), visited);
    free_node(node->get_high(), visited);

    delete node;
}