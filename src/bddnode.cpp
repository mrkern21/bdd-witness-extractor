#include "bddnode.hpp"

BDDNode::BDDNode(int var, int layer, std::vector<std::vector<int>>& unsatClauses) : unsatClauses(unsatClauses){
    this->var = var;
    this->layer = layer;
    this->unsatClauses = unsatClauses;
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