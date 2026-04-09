#include "internal.hpp"
#include "bdd.hpp"

class BDDCreator {

    private:
        std::vector<std::vector<int>> clauses;
        std::queue<int> varorder;
        
    public:
        BDDCreator(std::vector<std::vector<int>> clauses,
                   std::queue<int> varorder);
        BDD* createBDD();
};