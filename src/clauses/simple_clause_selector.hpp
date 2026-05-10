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

#endif