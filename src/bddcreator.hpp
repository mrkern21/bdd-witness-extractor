#include "internal.hpp"
#include "bdd.hpp"
#include "bddnode.hpp"
#include "util.hpp"

class BDDCreator {

    private:
        const int num_of_layers;
        const bool domain[2] = {true, false};
        std::vector<std::vector<int>>& clauses;
        std::vector<int>& varorder;
        std::vector<std::vector<Working_Node>> layers;
        std::vector<std::unordered_map<std::string, BDDNode*>> state_table;
        std::unordered_map<int, int> var_to_layer;
        
        NextState compute_next_state(int var, bool value, BDDNode* current_node,
        std::unordered_map<int, int>& current_assignment, int next_layer);
        BDDNode* obtain_equivalent_node(NextState& state, int next_layer);
        std::vector<std::vector<int>> normalize_state(const std::vector<std::vector<int>>& state);
        std::string state_to_key(const std::vector<std::vector<int>>& state);
        bool variable_below_layer(int var, int layer);

    public:
        BDDCreator(std::vector<std::vector<int>>& clauses,
                   std::vector<int>& varorder);
        BDD create_bdd(void);
};