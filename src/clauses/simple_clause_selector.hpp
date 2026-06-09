#ifndef __SIMPLE_CLAUSE_SELECTOR_HPP
#define __SIMPLE_CLAUSE_SELECTOR_HPP

#include "clauseselector.hpp"

// Simple_Clause_Selector — baseline clause selection by length and count.
// Iterates the input clause set in order and selects the first
// number_of_clauses clauses whose length is at most clause_length. No
// structural or community analysis is performed; used as a simple baseline to
// bound BDD construction scope when the full formula is too large.
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