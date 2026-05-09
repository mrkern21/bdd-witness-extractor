#include "internal.hpp"
#include "bdd/bddcreator.hpp"
#include "clauses/clausecollector.hpp"
#include "varorder/simple_varorder.hpp"
#include "clauses/simple_clause_selector.hpp"
#include "bdd/bdd.hpp"
#include <random>

int main(int argc, char* argv[]){

    std::vector<int> test_var_order = {1,2,3,4,5};
    std::vector<int> reversed_order = {5,4,3,2,1};
    CaDiCaL::Solver solver;
    ClauseCollector collector;

    const char* path = "./test/02cff9bea08ed95b701d23e743e2ce13-unif-k4-r9.931-v185-c1837-S4628723751657252146.cnf";
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

    Simple_Clause_Selector selector(clauses, 15, 5);

    std::vector<std::vector<int>> selected_clauses = selector.select_clauses();

    std::cout << "number of selected clauses: " << selected_clauses.size() << std::endl;

    SimpleVarOrder var_order_creator(selected_clauses);
    std::vector<int> simple_var_order = var_order_creator.create_var_order();

    //randomly shuffer var order
    std::mt19937 rng(std::random_device{}());
    std::shuffle(simple_var_order.begin(), simple_var_order.end(), rng);

    std::cout << "var order: " << clause_to_string(simple_var_order) << std::endl;

    std::cout << "number of distinct vars in clauses: " << simple_var_order.size() << std::endl;

    BDDCreator creator(selected_clauses, simple_var_order);

    BDD created_bdd = creator.create_bdd();

    bdd_stats stats = created_bdd.get_bdd_stats();

    print_bdd_stats(stats);

    std::vector<std::vector<int>> witness_clauses = created_bdd.generate_witness_clauses();
    std::cout << "number of obtained witness clauses: " << witness_clauses.size() << std::endl;

    //print witness_clauses
    print_witness_clauses(witness_clauses);

    for(const std::vector<int>& cl : witness_clauses){
        solver.clause(cl);
    }

    ClauseCollector collector_check;
    solver.traverse_clauses(collector_check);
    std::vector<std::vector<int>> clauses_check = collector_check.retrieve_clauses();
    std::cout << "number of clauses after witness clauses have been added: " << clauses_check.size() << std::endl;

    /* SAT SOLVING 
    int result_solve = solver.solve();

    print_sat_result(result_solve);
    */
    return 0;
}