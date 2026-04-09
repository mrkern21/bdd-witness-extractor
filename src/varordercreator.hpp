#include "internal.hpp"

class VarOrderCreator {

    public:
        virtual std::queue<int> createVarOrder() = 0;
};