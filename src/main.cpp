#include "internal.hpp"
#include "bddcreator.hpp"
#include "bdd.hpp"

int main(int argc, char* argv[]){

    std::vector<std::vector<int>> test_clauses = {
        { 3,  4,  5}, // (1) x3 v x4 v x5
        {-3, -4, -5}, // (2) ~x3 v ~x4 v ~x5
        {-3, -4},     // (3) ~x3 v ~x4
        {-3, -5},     // (4) ~x3 v ~x5
        {-4, -5},     // (5) ~x4 v ~x5
        { 1,  2},     // (6) x1 v x2
        {-1, -2},     // (7) ~x1 v ~x2
        { 1,  4,  5}, // (8) x1 v x4 v x5
        {-1,  3,  5}, // (9) ~x1 v x3 v x5
        { 2,  4,  5}, // (10) x2 v x4 v x5
        {-2,  3,  5}  // (11) ~x2 v x3 v x5
    };

    std::vector<std::vector<int>> empty_clauses = {};

    std::vector<int> test_var_order = {1,2,3,4,5};

    BDDCreator creator(test_clauses, test_var_order);

    BDD created_bdd = creator.create_bdd();

    std::cout << created_bdd.to_string() << std::endl;

    return 0;
}