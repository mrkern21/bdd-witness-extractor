#include "bddnode.hpp"

BDDNode::BDDNode(int var, int layer, std::vector<std::vector<int>>& unsatClauses, std::vector<std::vector<int>>& partialAssignment)
 : unsatClauses(unsatClauses), partialAssignment(partialAssignment){
    this->var = var;
    this->layer = layer;
    this->unsatClauses = unsatClauses;
    this->partialAssignment = partialAssignment;
}

BDDNode::~BDDNode(){
    delete this->low;
    delete this->high;
}

void BDDNode::setHigh(BDDNode* node){
    this->high = node;
}

void BDDNode::setLow(BDDNode* node){
    this->low = node;
}

void BDDNode::setParent(BDDNode* node){
    this->parent = node;
}

std::vector<std::vector<int>>& BDDNode::getUnsatClauses() const {
    return this->unsatClauses;
}

int BDDNode::getVar() const {
    return this->var;
}

int BDDNode::getLayer() const {
    return this->layer;
}

bool BDDNode::isInfeasible() const {
    return this->infeasible;
}

bool BDDNode::isUnsat() const {
    return this->unsat;
}