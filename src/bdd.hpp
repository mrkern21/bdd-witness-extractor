#ifndef __BDD_HPP
#define __BDD_HPP

#include "internal.hpp"
#include "bddnode.hpp"

class BDD {
    private:
        BDDNode* root = nullptr;
        std::vector<BDDNode*>& unsat_nodes;
        void traverse_pre_order(const BDDNode* node, std::string& result, std::unordered_set<const BDDNode*>& visited) const;


    public:
        BDD(BDDNode* root, std::vector<BDDNode*>& unsat_nodes);
        ~BDD();
        std::vector<std::vector<int>> generate_witness_clauses();
        std::string to_string() const;
};

#endif