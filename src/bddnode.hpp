#include "internal.hpp"

class BDDNode {

    private:
        BDDNode* parent = nullptr;
        BDDNode* low = nullptr;
        BDDNode* high = nullptr;
        std::vector<std::vector<int>>& unsatClauses;
        int var;
        int layer;
        bool isFalseSink;

    public:
        BDDNode(int var, int layer, std::vector<std::vector<int>>& unsatClauses);
        ~BDDNode();
        void setLow(BDDNode* node);
        void setHigh(BDDNode* node);
        void setParent(BDDNode* node);
};