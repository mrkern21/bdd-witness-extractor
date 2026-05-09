#include "internal.hpp"
#include "bdd/bddcreator.hpp"
#include "clauses/clausecollector.hpp"
#include "varorder/simple_varorder.hpp"
#include "varorder/kondylidou_var_order.hpp"
#include "clauses/simple_clause_selector.hpp"
#include "clauses/vig_clause_selector.hpp"
#include "bdd/bdd.hpp"

int main(int argc, char *argv[])
{
    CaDiCaL::Solver solver;
    ClauseCollector collector;

    const char *path = "./test/06e46ab0acac57a61638cbb741949a98-complete-500-0.1-1-9876543215001.cnf";
    int num_of_vars;

    const char *msg = solver.read_dimacs(path, num_of_vars);

    if (msg != 0)
    {
        std::cerr << msg << std::endl;
        return 1;
    }

    bool res = solver.traverse_clauses(collector);

    if (!res)
    {
        std::cerr << "clauses could not be traversed!" << std::endl;
        return 1;
    }

    std::vector<std::vector<int>> clauses = collector.retrieve_clauses();

    std::cout << "clauses: " << clauses.size() << std::endl;

    // TODO: think about heuristic for splitting the clauses into subsets
    // different subsets might also produce different results but this is a design decision to be made
    // for current instance try 10 chunks

    std::vector<std::vector<int>> witness_clauses;

    // split clauses before creating the BDD
    // std::vector<std::vector<std::vector<int>>> chunks = split(clauses, 10);
    // splitiing the instance into chunks before community detection lead to subsets that were too small to produce witness clauses

    // IDEA: maybe split into chunks if detected community is too large for BDD construction

    int clause_counter = 0;

    // Simple_Clause_Selector selector(clauses, 15, 5);
    VIG_Clause_Selector selector(clauses);

    while (selector.has_remaining_community())
    {
        std::vector<std::vector<int>> selected_clauses = selector.select_clauses();
        std::cout << "number of selected clauses: " << selected_clauses.size() << std::endl;
        clause_counter += static_cast<int>(selected_clauses.size());

        // SimpleVarOrder var_order_creator(selected_clauses);
        // std::vector<int> simple_var_order = var_order_creator.create_var_order();

        // randomly shuffer var order
        // AVOID SHUFFLE FOR NOW TO PRODUCE DETERMINISTIC RESULTS!!
        // std::shuffle(simple_var_order.begin(), simple_var_order.end(), rng);
        // std::mt19937 rng(std::random_device{}());

        // if the selected community is too large, i.e., larger than 300 clauses split the community
        auto chunks = split_community(selected_clauses);

        for (std::vector<std::vector<int>> &chunk : chunks)
        {
            
            std::cout << "number of clauses in current chunk: " << chunk.size() << std::endl;
            Kondy_VarOrder var_order_creator(chunk);
            std::vector<int> var_order = var_order_creator.create_var_order();

            std::cout << "var order: " << clause_to_string(var_order) << std::endl;

            std::cout << "number of distinct vars in clauses: " << var_order.size() << std::endl;

            BDDCreator creator(chunk, var_order);

            BDD created_bdd = creator.create_bdd();

            bdd_stats stats = created_bdd.get_bdd_stats();

            //print_bdd_stats(stats);

            std::vector<std::vector<int>> inner_witness_clauses = created_bdd.generate_witness_clauses();
            std::cout << "number of obtained witness clauses: " << inner_witness_clauses.size() << std::endl;

            print_witness_clauses(inner_witness_clauses);

            witness_clauses.insert(witness_clauses.end(), inner_witness_clauses.begin(), inner_witness_clauses.end());
        }
    }

    std::cout << std::endl;
    std::cout << "number of total witness clauses: " << witness_clauses.size() << std::endl;
    std::cout << "number of inspected clauses: " << clause_counter << std::endl;

    for (const std::vector<int> &cl : witness_clauses)
    {
        solver.clause(cl);
    }

    ClauseCollector collector_check;
    solver.traverse_clauses(collector_check);
    std::vector<std::vector<int>> clauses_check = collector_check.retrieve_clauses();
    std::cout << "number of clauses after witness clauses have been added: " << clauses_check.size() << std::endl;

    // SAT SOLVING
    int result_solve = solver.solve();

    print_sat_result(result_solve);

    return 0;
}