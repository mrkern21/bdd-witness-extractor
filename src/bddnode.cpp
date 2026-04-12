#include "bddnode.hpp"

BDDNode::BDDNode(int var, int layer, std::vector<std::vector<int>> unsat_clauses)
 : unsat_clauses(std::move(unsat_clauses)){
    this->var = var;
    this->layer = layer;
    this->unsat = false;
    this->infeasible = false;
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

void BDDNode::set_parent(BDDNode* node){
    this->parent = node;
}

std::vector<std::vector<int>> BDDNode::get_unsat_clauses() const {
    return this->unsat_clauses;
}

int BDDNode::get_var() const {
    return this->var;
}

int BDDNode::get_layer() const {
    return this->layer;
}

bool BDDNode::is_infeasible() const {
    return this->infeasible;
}

bool BDDNode::is_unsat() const {
    return this->unsat;
}

BDDNode* BDDNode::get_high() const {
    return this->high;
}

BDDNode* BDDNode::get_low() const {
    return this->low;
}

std::string BDDNode::get_state() const {
    std::string result;

    result += "{";
    for(const std::vector<int>& clause : unsat_clauses){
        result += "[";
        for(int lit : clause){
            result += std::to_string(lit) + " ";
        }
        result += "]";
    }
    result += "}";

    return result;
}