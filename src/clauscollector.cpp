/**#include "clausecollector.hpp"

bool ClauseCollector::clause(const std::vector<int> & cl){
    this->clauses.push_back(cl);
    return true;
}

const std::vector<std::vector<int>> ClauseCollector::retrieveClauses() const {
    return this->clauses;
}**/