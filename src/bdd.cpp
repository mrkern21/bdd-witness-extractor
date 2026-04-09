#include "bdd.hpp"

BDD::BDD(){}

BDD::~BDD(){
    delete this->root;
}

std::vector<std::vector<int>>* BDD::generateWitnessClauses(){
    return nullptr;
}