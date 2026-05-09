#ifndef __SIMPLE_CLAUSE_SELECTOR_HPP
#define __SIMPLE_CLAUSE_SELECTOR_HPP

#include "clauseselector.hpp"

//only select max number_of_clauses clauses up to length clause_length (and observe what happens)
class Simple_Clause_Selector : public ClauseSelector {
    private:
        int number_of_clauses;
        int clause_length;
        std::vector<std::vector<int>>& clauses;

    public:
        Simple_Clause_Selector(std::vector<std::vector<int>>& clauses, int number_of_clauses, int clause_length);
        std::vector<std::vector<int>> select_clauses() override;
};

Simple_Clause_Selector::Simple_Clause_Selector(std::vector<std::vector<int>>& clauses, int number_of_clauses, int clause_length) : clauses(clauses)
{
    this->number_of_clauses = number_of_clauses;
    this->clause_length = clause_length;
}


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

#endif