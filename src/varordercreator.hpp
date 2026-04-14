#ifndef __VARORDER_HPP
#define __VARORDER_HPP

#include "internal.hpp"

class VarOrderCreator {

    public:
        virtual std::vector<int>& create_var_order() = 0;
};

#endif