#include "internal.hpp"
#include "bdd.hpp"
#include "bddnode.hpp"

class BDDCreator {

    private:
        std::vector<std::vector<int>>& clauses;
        std::queue<int>& varorder;
        int num_of_layers;
        std::vector<std::vector<BDDNode*>> layers;
        const bool domain[2] = {true, false};

        std::vector<std::vector<int>> computeNextState(int var, bool value, BDDNode* currentNode);
        BDDNode* obtainEquivalentNode(std::vector<std::vector<int>>& computedState, int nextLayer);
        void checkUnsat(void);
        void checkInfeasible(void);

    public:
        BDDCreator(std::vector<std::vector<int>>& clauses,
                   std::queue<int>& varorder);
        BDD createBDD(void);
};