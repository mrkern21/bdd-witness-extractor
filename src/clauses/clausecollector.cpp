#include "clausecollector.hpp"

// Called by CaDiCaL once per clause during traverse_clauses(); must return
// true to signal that traversal should continue.
bool ClauseCollector::clause(const std::vector<int> & cl){
    this->clauses.push_back(cl);
    return true;
}

// Returns the full clause set accumulated across all clause() callbacks.
const std::vector<std::vector<int>> ClauseCollector::retrieve_clauses() const {
    return this->clauses;
}