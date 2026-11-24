// BDD — Binary Decision Diagram built over a set of CNF clauses using a fixed
// variable ordering. Nodes are organized in layers (one per variable,
// plus a final layer for the nodes reached after the last variable);
// Each internal node branches on its variable (low = false, high = true) and is
// classified as DEFAULT, SAT, UNSAT, or INFEASIBLE during construction.
// The primary output is generate_witness_clauses(), which collects witness
// clauses from INFEASIBLE nodes that have no infeasible parent. These clauses
// are derived via resolution during BDD construction and encode the partial
// assignments that cause infeasibility; they are appended to the original CNF
// to steer the SAT solver away from unsatisfiable regions.
#ifndef __BDD_HPP
#define __BDD_HPP

#include "internal.hpp"
#include "bddnode.hpp"
#include "../util.hpp"
#include <set>

class BDD {
    private:
        BDDNode* root = nullptr;
        std::vector<std::vector<BDDNode*>> layers;
        
        void traverse_pre_order(const BDDNode* node, std::string& result, std::unordered_set<const BDDNode*>& visited) const;

    public:
        BDD(BDDNode* root, std::vector<std::vector<BDDNode*>> layers);
        ~BDD();

        std::vector<std::vector<int>> generate_witness_clauses(void);
        std::string to_string(void) const;
        bdd_stats get_bdd_stats(void) const;
};

#endif