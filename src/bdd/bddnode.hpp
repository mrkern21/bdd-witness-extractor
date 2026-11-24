// BDDNode — a single node in the BDD DAG.
// Each node branches on one variable (var) at a fixed layer index, with a low
// child (var = false) and a high child (var = true). unsat_clauses holds the
// clauses not yet satisfied by the partial assignment that leads to this node;
// this set is the node's identity for merging equivalent nodes during
// construction. status is one of DEFAULT, SAT, UNSAT, or INFEASIBLE:
//   SAT        — all clauses satisfied; this path is a solution.
//   UNSAT      — at least one clause has no remaining unassigned literal.
//   INFEASIBLE — both children are UNSAT/INFEASIBLE; witness_clause holds the
//                resolution-derived clause that proves no extension of this
//                partial assignment can satisfy the formula.
// has_infeasible_parent is set during the bottom-up pass to mark that this
// node is already ruled out by an ancestor's witness clause, preventing
// redundant witness collection in BDD::generate_witness_clauses().
#ifndef __BDDNODE_HPP
#define __BDDNODE_HPP

#include "../internal.hpp"
#include "../util.hpp"

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
        BDDNode(int var, int layer, std::vector<std::vector<int>> unsat_clauses, NodeStatus status, std::vector<int> witness_clause);
        ~BDDNode();
        
        void set_low(BDDNode* node);
        void set_high(BDDNode* node);
        void set_parent(BDDNode* node);
        void set_status(NodeStatus status);
        void set_infeasible_parent(bool value);
        void set_witness_clause(std::vector<int> witness_clause);

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