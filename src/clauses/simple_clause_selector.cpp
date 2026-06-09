#include "simple_clause_selector.hpp"

Simple_Clause_Selector::Simple_Clause_Selector(std::vector<std::vector<int>>& clauses, int number_of_clauses, int clause_length) : clauses(clauses)
{
    this->number_of_clauses = number_of_clauses;
    this->clause_length = clause_length;
}

// Iterates clauses in their original order, collecting only those whose
// length does not exceed clause_length, and stops once number_of_clauses
// have been collected. The in-order scan means short leading clauses are
// preferred; no structural selection is performed.
std::vector<std::vector<int>> Simple_Clause_Selector::select_clauses()
{
    std::vector<std::vector<int>> result;
    int clause_add_counter = 0;
    int counter = 0;

    for(std::vector<int>& clause : clauses){

        counter++;
        //skip the first 1000 clauses
        //if(counter < 1000) continue;

        if(clause_add_counter >= number_of_clauses){
            break;
        }
        
        if((int)clause.size() <= clause_length){
            std::vector<int> copy_clause = clause;
            result.push_back(copy_clause);
            clause_add_counter++;
        }
        
    }

    return result;
}