#include "internal.hpp"
#include "bdd.hpp"
#include "bddnode.hpp"
#include "nodestatus.hpp"

class BDDCreator {

    private:
        const int num_of_layers;
        const bool domain[2] = {true, false};
        std::vector<std::vector<int>>& clauses;
        std::queue<int>& varorder;
        std::vector<std::vector<BDDNode*>> layers;
        std::vector<std::unordered_map<std::string, BDDNode*>> state_table;
        
        std::vector<std::vector<int>> compute_next_state(int var, bool value, BDDNode* current_node);
        BDDNode* obtain_equivalent_node(const std::vector<std::vector<int>>& computed_state, int next_layer);
        std::vector<std::vector<int>> normalize_state(const std::vector<std::vector<int>>& state);
        std::string state_to_key(const std::vector<std::vector<int>>& state);

    public:
        BDDCreator(std::vector<std::vector<int>>& clauses,
                   std::queue<int>& varorder);
        BDD create_bdd(void);
};