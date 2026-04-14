#include "bddnode.hpp"

BDDNode::BDDNode(int var, int layer, std::vector<std::vector<int>> unsat_clauses, NodeStatus status)
 : unsat_clauses(std::move(unsat_clauses)){
    this->var = var;
    this->layer = layer;
    this->status = status;
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