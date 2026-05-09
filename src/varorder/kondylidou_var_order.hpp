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