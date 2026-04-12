#include "clausecollector.hpp"

bool ClauseCollector::clause(const std::vector<int> & cl){
    this->clauses.push_back(cl);
    return true;
}

const std::vector<std::vector<int>> ClauseCollector::retrieve_clauses() const {
    return this->clauses;
}