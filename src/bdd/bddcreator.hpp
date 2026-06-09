// BDDCreator — constructs a BDD from a CNF formula and a variable ordering.
// Construction is two-phase: top_down_construction() builds the DAG layer by
// layer, where each node's state is the set of clauses not yet satisfied by the
// partial assignment leading to it. Nodes with identical normalized states are
// merged via a per-layer hash table (state_table) to keep the graph compact.
// A node becomes UNSAT when an unsatisfied clause runs out of unassigned
// literals, and SAT when all clauses are satisfied.
// bottom_up_pass() then propagates INFEASIBLE status upward: a node becomes
// INFEASIBLE when both children are UNSAT or INFEASIBLE, and its witness clause
// is computed via resolution over the children's witness clauses, eliminating
// the branching variable. Construction aborts if the node count exceeds
// MAX_NODES; the caller is expected to catch the resulting runtime_error.
#ifndef __BDDCREATOR_HPP
#define __BDDCREATOR_HPP

#include "../internal.hpp"
#include "bdd.hpp"
#include "bddnode.hpp"
#include "../util.hpp"

class BDDCreator {

    private:
        const int num_of_layers;
        const bool domain[2] = {true, false};
        std::vector<std::vector<int>>& clauses;
        std::vector<int>& varorder;
        std::vector<std::vector<BDDNode*>> layers;
        std::vector<std::unordered_map<std::string, BDDNode*>> state_table;
        std::unordered_map<int, int> var_to_layer;
        int num_of_nodes;
        int MAX_NODES = 800000;
        bool ownership_transferred = false;
        
        BDDNode* top_down_construction(void);
        void bottom_up_pass(void);

        NextState compute_next_state(int var, bool value, BDDNode* current_node, int next_layer);
        BDDNode* obtain_equivalent_node(NextState& state, int next_layer);
        std::vector<std::vector<int>> normalize_state(const std::vector<std::vector<int>>& state);
        std::string state_to_key(const std::vector<std::vector<int>>& state);
        bool variable_below_layer(int var, int layer);
        bool var_in_clause(int var, const std::vector<int>& clause);
        std::vector<int> apply_resolution(const std::vector<int>& low_witness, const std::vector<int>& high_witness, int var);

    public:
        BDDCreator(std::vector<std::vector<int>>& clauses,
                   std::vector<int>& varorder,
                   int MAX_NODES);
        ~BDDCreator();
        BDD create_bdd(void);
};

#endif