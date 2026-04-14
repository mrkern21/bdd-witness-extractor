#ifndef __BDDNODE_HPP
#define __BDDNODE_HPP

#include "internal.hpp"
#include "util.hpp"

class BDDNode {

    private:
        BDDNode* parent = nullptr;
        BDDNode* low = nullptr;
        BDDNode* high = nullptr;
        std::vector<std::vector<int>> unsat_clauses;
        int var;
        int layer;
        NodeStatus status;

    public:
        BDDNode(int var, int layer, std::vector<std::vector<int>> unsat_clauses, NodeStatus status);
        ~BDDNode();
        
        void set_low(BDDNode* node);
        void set_high(BDDNode* node);
        void set_parent(BDDNode* node);
        void set_status(NodeStatus status);

        std::vector<std::vector<int>> get_unsat_clauses() const;
        int get_var() const;
        int get_layer() const;
        BDDNode* get_low() const;
        BDDNode* get_high() const;
        std::string get_state() const;
        NodeStatus get_status() const;
};

#endif