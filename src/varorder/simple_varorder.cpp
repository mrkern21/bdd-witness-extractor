#include "simple_varorder.hpp"

SimpleVarOrder::SimpleVarOrder(std::vector<std::vector<int>>& clauses) : clauses(clauses)
{
}

std::vector<int> SimpleVarOrder::create_var_order() {
    std::unordered_set<int> set;
    for(std::vector<int>& clause : clauses){
        for(int lit : clause){
            int var = std::abs(lit);
            set.insert(var);
        }
    }
    return std::vector<int>(set.begin(), set.end());
}