#ifndef __VARORDER_HPP
#define __VARORDER_HPP

#include "internal.hpp"

class VarOrderCreator {

    public:
        virtual std::queue<int>& createVarOrder() = 0;
};

#endif