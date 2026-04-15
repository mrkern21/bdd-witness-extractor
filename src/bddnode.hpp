#ifndef __BDDNODE_HPP
#define __BDDNODE_HPP

#include "internal.hpp"
#include "util.hpp"

class BDDNode {

    private:
        BDDNode* low = nullptr;
        BDDNode* high = nullptr;
        std::vector<std::vector<int>> unsat_clauses;
        std::vector<int> witness_clause;
        int var;
        int layer;
        NodeStatus status;
        bool has_infeasible_parent;

    public:
        BDDNode(int var, int layer, std::vector<std::vector<int>> unsat_clauses, NodeStatus status);
        ~BDDNode();
        
        void set_low(BDDNode* node);
        void set_high(BDDNode* node);
        void set_parent(BDDNode* node);
        void set_status(NodeStatus status);
        void set_witness_clause(std::vector<int> clause);
        void set_infeasible_parent(bool value);

        std::vector<std::vector<int>> get_unsat_clauses(void) const;
        std::vector<int> get_witness_clause(void) const;
        int get_var(void) const;
        int get_layer(void) const;
        BDDNode* get_low(void) const;
        BDDNode* get_high(void) const;
        std::string get_state(void) const;
        NodeStatus get_status(void) const;
        bool get_has_infeasible_parent(void) const;
};

#endif