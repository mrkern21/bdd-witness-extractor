#include "internal.hpp"
#include "bddcreator.hpp"
#include "clausecollector.hpp"
#include "simple_varorder.hpp"
#include "bdd.hpp"

int main(int argc, char* argv[]){

    std::vector<int> test_var_order = {1,2,3,4,5};
    std::vector<int> reversed_order = {5,4,3,2,1};
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
    std::cout << "number of clauses before witness generation: " << clauses.size() << std::endl;

    std::cout << "clauses: " << clauses.size() << std::endl;

    SimpleVarOrder var_order_creator(clauses);
    std::vector<int> simple_var_order = var_order_creator.create_var_order();

    std::cout << "var order: " << clause_to_string(simple_var_order) << std::endl;

    std::cout << "number of distinct vars in clauses: " << simple_var_order.size() << std::endl;

    BDDCreator creator(clauses, reversed_order);

    BDD created_bdd = creator.create_bdd();

    bdd_stats stats = created_bdd.number_of_nodes();
    std::cout << "BDD stats: num_of_nodes: " << stats.num_of_nodes << std::endl
            << "num_of_default_nodes: " << stats.num_of_default_nodes << std::endl
            << "num_of_sat_nodes: " << stats.num_of_sat_nodes << std::endl
            << "num_of_unsat_nodes: " << stats.num_of_unsat_nodes << std::endl
            <<"num_of_infeasible_nodes: " << stats.num_of_infeasible_nodes << std::endl;

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

    int result_solve = solver.solve();

    print_sat_result(result_solve);

    return 0;
}