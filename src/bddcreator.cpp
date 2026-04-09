#include "bddcreator.hpp"

BDDCreator::BDDCreator(std::vector<std::vector<int>> clauses,
                   std::queue<int> varorder) {
    this->clauses = clauses;
    this->varorder = varorder;                    
}

BDD* BDDCreator::createBDD(){
    return nullptr;
}