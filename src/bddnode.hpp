#ifndef __BDDNODE_HPP
#define __BDDNODE_HPP

#include "internal.hpp"

class BDDNode {

    private:
        BDDNode* parent = nullptr;
        BDDNode* low = nullptr;
        BDDNode* high = nullptr;
        std::vector<std::vector<int>>& unsat_clauses;
        int var;
        int layer;
        bool infeasible;
        bool unsat;

    public:
        BDDNode(int var, int layer, std::vector<std::vector<int>>& unsat_clauses);
        ~BDDNode();
        
        void set_low(BDDNode* node);
        void set_high(BDDNode* node);
        void set_parent(BDDNode* node);

        std::vector<std::vector<int>>& get_unsat_clauses() const;
        int get_var() const;
        int get_layer() const;
        bool is_infeasible() const;
        bool is_unsat() const;
        BDDNode* get_low() const;
        BDDNode* get_high() const;
};

#endif