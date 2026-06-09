// VarOrderCreator — abstract base for variable ordering strategies.
// Subclasses produce a vector of variable ids representing the top-to-bottom
// layer assignment for BDD construction. The ordering directly affects BDD
// size: a good order places high-constraint variables near the root to prune
// infeasible branches early.
// Concrete implementations: Kondy_VarOrder (heuristic), SimpleVarOrder (baseline).
#ifndef __VARORDER_HPP
#define __VARORDER_HPP

#include "../internal.hpp"

class VarOrderCreator {

    public:
        virtual std::vector<int> create_var_order() = 0;
};

#endif