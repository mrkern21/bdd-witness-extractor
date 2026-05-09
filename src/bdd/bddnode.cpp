#include "bddnode.hpp"

BDDNode::BDDNode(int var, int layer, std::vector<std::vector<int>> unsat_clauses, NodeStatus status, std::vector<int> witness_clause)
 : unsat_clauses(std::move(unsat_clauses)), witness_clause(std::move(witness_clause)){
    this->var = var;
    this->layer = layer;
    this->status = status;
    this->has_infeasible_parent = false;
}

BDDNode::~BDDNode(){
    #ifdef DEBUG
        std::cerr << "Deleting node " << this
              << " var=" << var
              << " layer=" << layer << "\n";
    #endif
}

void BDDNode::set_high(BDDNode* node){
    this->high = node;
}

void BDDNode::set_low(BDDNode* node){
    this->low = node;
}

void BDDNode::set_status(NodeStatus status){
    this->status = status;
}

void BDDNode::set_infeasible_parent(bool value){
    this->has_infeasible_parent = value;
}

void BDDNode::set_witness_clause(std::vector<int> witness_clause){
    this->witness_clause = witness_clause;
}

std::vector<std::vector<int>> BDDNode::get_unsat_clauses(void) const {
    return this->unsat_clauses;
}

std::vector<int> BDDNode::get_witness_clause(void) const {
    return this->witness_clause;
}

int BDDNode::get_var(void) const {
    return this->var;
}

int BDDNode::get_layer(void) const {
    return this->layer;
}

BDDNode* BDDNode::get_high(void) const {
    return this->high;
}

BDDNode* BDDNode::get_low(void) const {
    return this->low;
}

NodeStatus BDDNode::get_status(void) const {
    return this->status;
}

std::string BDDNode::get_state(void) const {
    std::string result;

    result += "{";
    for(const std::vector<int>& clause : unsat_clauses){
        result += "[";
        for(int lit : clause){
            result += std::to_string(lit) + " ";
        }
        result.pop_back();
        result += "]";
    }
    result += "}";

    return result;
}

bool BDDNode::get_has_infeasible_parent(void) const {
    return this->has_infeasible_parent;
}