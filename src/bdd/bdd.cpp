#include "bdd.hpp"

// Ownership of all BDDNode heap allocations is transferred here from BDDCreator.
BDD::BDD(BDDNode* root, std::vector<std::vector<BDDNode*>> layers) : root(root), layers(std::move(layers))
{
}

// Deletes every node layer by layer; the layers vector is the sole ownership record.
BDD::~BDD() {
    for(size_t current_layer = 0; current_layer < layers.size(); current_layer++){
        for(BDDNode* node : layers[current_layer]){
            delete node;
        }
    }
}

// Collects one witness clause per top-level INFEASIBLE subtree.
// Only nodes without an infeasible parent are collected: nodes with an
// infeasible parent have already been subsumed by an ancestor's witness clause,
// so including them would produce redundant clauses.
// Nodes whose resolution produced a tautological resolvent have an empty
// witness clause; these are skipped to avoid emitting an empty clause into
// the CNF (which DIMACS parsers interpret as immediately UNSAT).
// Deduplication via std::set is required because shared (merged) nodes can
// be reached as a child of multiple parents, potentially adding the same
// witness clause more than once.
std::vector<std::vector<int>> BDD::generate_witness_clauses(void) {
    std::vector<std::vector<int>> result;
    for(size_t current_layer = 0; current_layer < layers.size(); current_layer++){
        for(BDDNode* node : layers[current_layer]){
            if(node->get_status() == NodeStatus::INFEASIBLE && !node->get_has_infeasible_parent()){
                std::vector<int> witness_clause = node->get_witness_clause();
                if(!witness_clause.empty()){
                    result.push_back(witness_clause);
                }
            }
        }
    }

    std::set<std::vector<int>> s(result.begin(), result.end());
    result.assign(s.begin(), s.end());

    return result;
}

// Produces a human-readable dump of the entire DAG for debugging.
std::string BDD::to_string(void) const {
    std::string result;
    std::unordered_set<const BDDNode*> visited;

    traverse_pre_order(this->root, result, visited);

    result += "total number of nodes: " + std::to_string(visited.size()) + "\n";

    return result;
}

// Pre-order DAG traversal; the visited set guards against revisiting shared
// nodes (the BDD is a DAG, not a tree, so a node can be the child of multiple
// parents after state-merging in BDDCreator).
// var is printed as "none" for terminal nodes (SAT/UNSAT) and for nodes whose
// var field was set to -1 at the last layer boundary.
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

// Iterates the full layers vector (authoritative node list) rather than
// traversing the DAG, so every node is counted exactly once regardless of
// how many parents point to it.
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