// Kondy_VarOrder — variable ordering heuristic based on Kondylidou's master thesis.
// Scores each variable as (number of clauses it appears in) / (average length
// of those clauses). Variables with high scores — appearing in many short
// clauses — are placed at the top BDD layers, since short clauses constrain
// more paths early and tend to produce smaller BDDs by pruning infeasible
// branches sooner. Ties are broken by ascending variable id for determinism.
#ifndef __KONDY_VAR_ORDER_HPP
#define __KONDY_VAR_ORDER_HPP

#include "../internal.hpp"
#include "varordercreator.hpp"

class Kondy_VarOrder : public VarOrderCreator{

    private:
        std::vector<std::vector<int>>& clauses;

    public:
        Kondy_VarOrder(std::vector<std::vector<int>>& clauses);
        std::vector<int> create_var_order() override;

};

#endif