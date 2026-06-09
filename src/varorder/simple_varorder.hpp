// SimpleVarOrder — baseline variable ordering with no heuristic.
// Collects all distinct variables appearing in the clause set via a hash set,
// returning them in unspecified (hash-table iteration) order. Used as a
// reference baseline to compare against heuristic orderings such as
// Kondy_VarOrder.
#ifndef __SIMPLE_VARORDER_HPP
#define __SIMPLE_VARORDER_HPP

#include "varordercreator.hpp"
#include <unordered_set>

class SimpleVarOrder : public VarOrderCreator {

    private:
        std::vector<std::vector<int>>& clauses;

    public:
        SimpleVarOrder(std::vector<std::vector<int>>& clauses);
        std::vector<int> create_var_order() override;
};

#endif