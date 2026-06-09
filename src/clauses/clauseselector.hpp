// ClauseSelector — abstract base for clause selection strategies.
// Controls which subset of the full clause set is passed to BDD construction.
// Concrete subclasses implement different selection policies:
//   Simple_Clause_Selector — length-capped prefix filter (baseline).
//   VIG_Clause_Selector    — community-based selection via VIG + Louvain.
#ifndef __CLAUSESELECTOR_HPP
#define __CLAUSESELECTOR_HPP

#include "../internal.hpp"

class ClauseSelector {

    public:
        virtual std::vector<std::vector<int>> select_clauses() = 0;
};

#endif