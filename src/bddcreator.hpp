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

        std::vector<std::vector<int>> compute_next_state(int var, bool value, BDDNode* current_node);
        BDDNode* obtain_equivalent_node(std::vector<std::vector<int>>& computed_state, int next_layer);

    public:
        BDDCreator(std::vector<std::vector<int>>& clauses,
                   std::queue<int>& varorder);
        BDD create_bdd(void);
};