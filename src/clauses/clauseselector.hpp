#ifndef __CLAUSESELECTOR_HPP
#define __CLAUSESELECTOR_HPP

#include "../internal.hpp"

class ClauseSelector {

    public:
        virtual std::vector<std::vector<int>> select_clauses() = 0;
};

#endif