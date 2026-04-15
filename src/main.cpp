#include "internal.hpp"
#include "bddcreator.hpp"
#include "clausecollector.hpp"
#include "bdd.hpp"

int main(int argc, char* argv[]){
/** 
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

    std::vector<std::vector<int>> random_clauses = {
        { -1, 3 },
        { 2, -5, 1 },
        { 3, 2, -1, 5 },
        { -2, -3 },
        { -5, 4, 3 },
        { 2, -1 },
        { -3, 5, -2, 1 },
        { -1, -2, -3 },
        { 5, 3 },
        { -4, 2 },
        { 1, -5 },
        { 3, -1, 4 },
        { 2, 3, -4, 1 },
        { 5, -3 }
    };

    std::vector<std::vector<int>> empty_clauses = {};
    std::vector<std::vector<int>> t2_clauses = {{1, 2}, {-1}};

    
    std::vector<int> test_var_order_2 = {5,4,3,2,1};
    
    */

    std::vector<int> test_var_order = {1,2,3,4,5};
    CaDiCaL::Solver solver;
    ClauseCollector collector;

    const char* path = "./test/kell_example.cnf";
    int num_of_vars;

    const char* msg = solver.read_dimacs(path, num_of_vars);

    if(msg != 0){
        std::cerr << msg << std::endl;
        return 1;
    }

    bool res = solver.traverse_clauses(collector);
    
    if(!res){
        std::cerr << "clauses could not be traversed!" << std::endl;
        return 1;
    }

    std::vector<std::vector<int>> clauses = collector.retrieve_clauses();
    
    std::cout << "clauses: " << clauses.size() << std::endl;

    BDDCreator creator(clauses, test_var_order);

    BDD created_bdd = creator.create_bdd();

    //std::cout << created_bdd.to_string() << std::endl;

    std::vector<std::vector<int>> witness_clauses = created_bdd.generate_witness_clauses();

    for(const std::vector<int>& cl : witness_clauses){
        solver.clause(cl);
    }

    ClauseCollector collector_check;
    solver.traverse_clauses(collector_check);
    std::vector<std::vector<int>> clauses_check = collector_check.retrieve_clauses();
    std::cout << "number of clauses after witness clauses have been added: " << clauses_check.size() << std::endl;

    while(solver.solve() == 10){
        
        std::vector<int> blocking_clause;
        for(int var : test_var_order){
            int val = solver.val(var);
            std::cout << var << ": " << val << ", ";
            blocking_clause.push_back(-val);
        }

        solver.clause(blocking_clause);
        std::cout << std::endl;
        std::cout << "SAT!" << std::endl;

    }
    
    return 0;
}