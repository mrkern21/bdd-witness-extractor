#ifndef __BDD_HPP
#define __BDD_HPP

#include "internal.hpp"
#include "bddnode.hpp"

class BDD {
    private:
        BDDNode* root = nullptr;
        std::vector<BDDNode*>& unsatNodes;
        void traversePreOrder(const BDDNode* node, std::string& result, std::unordered_set<const BDDNode*>& visited) const;


    public:
        BDD(BDDNode* root, std::vector<BDDNode*>& unsatNodes);
        ~BDD();
        std::vector<std::vector<int>> generateWitnessClauses();
        std::string toString() const;
};

#endif