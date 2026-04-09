#include "internal.hpp"
#include "bddnode.hpp"

class BDD {
    private:
        BDDNode* root = nullptr;

    public:
        BDD();
        ~BDD();

        std::vector<std::vector<int>>* generateWitnessClauses();
};