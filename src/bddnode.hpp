#ifndef __BDDNODE_HPP
#define __BDDNODE_HPP

#include "internal.hpp"

class BDDNode {

    private:
        BDDNode* parent = nullptr;
        BDDNode* low = nullptr;
        BDDNode* high = nullptr;
        std::vector<std::vector<int>>& unsatClauses;
        std::vector<std::vector<int>>& partialAssignment;
        int var;
        int layer;
        bool infeasible;
        bool unsat;

    public:
        BDDNode(int var, int layer, std::vector<std::vector<int>>& unsatClauses,
                std::vector<std::vector<int>>& partialAssignment);
        ~BDDNode();
        
        void setLow(BDDNode* node);
        void setHigh(BDDNode* node);
        void setParent(BDDNode* node);

        std::vector<std::vector<int>>& getUnsatClauses() const;
        int getVar() const;
        int getLayer() const;
        bool isInfeasible() const;
        bool isUnsat() const;
};

#endif