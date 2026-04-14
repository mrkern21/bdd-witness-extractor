#ifndef __BDD_HPP
#define __BDD_HPP

#include "internal.hpp"
#include "bddnode.hpp"
#include "util.hpp"

class BDD {
    private:
        BDDNode* root = nullptr;
        std::vector<std::vector<BDDNode*>> layers;

        void traverse_pre_order(const BDDNode* node, std::string& result, std::unordered_set<const BDDNode*>& visited) const;
        void free_node(BDDNode* node, std::unordered_set<BDDNode*>& visited);

    public:
        BDD(BDDNode* root, std::vector<std::vector<BDDNode*>> layers);
        ~BDD();

        std::vector<std::vector<int>> generate_witness_clauses(void);
        std::string to_string(void) const;
};

#endif