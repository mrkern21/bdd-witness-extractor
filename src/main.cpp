#include "internal.hpp"
#include "bdd/bddcreator.hpp"
#include "clauses/clausecollector.hpp"
#include "varorder/simple_varorder.hpp"
#include "varorder/kondylidou_var_order.hpp"
#include "clauses/simple_clause_selector.hpp"
#include "clauses/vig_clause_selector.hpp"
#include "bdd/bdd.hpp"

static bool try_build_and_extract(
    std::vector<std::vector<int>> &chunk,
    std::vector<std::vector<int>> &witness_clauses)
{
    std::cout << "number of clauses in current chunk: " << chunk.size() << std::endl;

    Kondy_VarOrder var_order_creator(chunk);
    std::vector<int> var_order = var_order_creator.create_var_order();
    std::cout << "var order: " << clause_to_string(var_order) << std::endl;
    std::cout << "number of distinct vars in clauses: " << var_order.size() << std::endl;

    try
    {
        BDDCreator creator(chunk, var_order);
        BDD created_bdd = creator.create_bdd();

        auto inner = created_bdd.generate_witness_clauses();
        std::cout << "number of obtained witness clauses: " << inner.size() << std::endl;
        print_witness_clauses(inner);

        witness_clauses.insert(witness_clauses.end(), inner.begin(), inner.end());
        return true;
    }
    catch (const std::runtime_error &e)
    {
        std::cout << "BDD construction failed: " << e.what() << std::endl;
        return false;
    }
}

int main(int argc, char *argv[])
{
    CaDiCaL::Solver solver;
    ClauseCollector collector;

    const char *path = "./test/sat_second_example.cnf";
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

    int clause_counter = 0;

    // TODO: think about heuristic for splitting the clauses into subsets
    // different subsets might also produce different results but this is a design decision to be made
    // for current instance try 10 chunks

    std::vector<std::vector<int>> witness_clauses;

    // split clauses before creating the BDD
    // std::vector<std::vector<std::vector<int>>> chunks = split(clauses, 10);
    // splitiing the instance into chunks before community detection lead to subsets that were too small to produce witness clauses

    // IDEA: maybe split into chunks if detected community is too large for BDD construction

    // SimpleVarOrder var_order_creator(selected_clauses);
    // std::vector<int> simple_var_order = var_order_creator.create_var_order();
    // randomly shuffer var order
    // AVOID SHUFFLE FOR NOW TO PRODUCE DETERMINISTIC RESULTS!!
    // std::shuffle(simple_var_order.begin(), simple_var_order.end(), rng);
    // std::mt19937 rng(std::random_device{}());

    
    // Fast path: small inputs go straight to BDD construction, no community
    // detection or splitting. The threshold matches what the BDD builder is
    // known to handle reliably.
    if (clauses.size() < 60)
    {
        clause_counter += static_cast<int> (clauses.size());
        try_build_and_extract(clauses, witness_clauses);
    }
    else
    {
        // Standard path: community detection, then split each community to fit
        // the BDD builder's empirical limits, with a retry-after-failure layer.
        
        VIG_Clause_Selector selector(clauses);

        while (selector.has_remaining_community())
        {
            std::vector<std::vector<int>> selected_clauses = selector.select_clauses();
            std::cout << "number of selected clauses: " << selected_clauses.size() << std::endl;
            clause_counter += static_cast<int>(selected_clauses.size());

            auto chunks = split_community(selected_clauses); // default 300 / 95
            for (std::vector<std::vector<int>> &chunk : chunks)
            {
                if (try_build_and_extract(chunk, witness_clauses))
                {
                    continue; // first attempt succeeded, on to the next chunk
                }

                // First attempt threw. Split harder and retry each sub-chunk.
                // Sub-chunks that also fail are simply skipped; we move on
                // rather than recurse further.
                std::cout << "first build failed; retrying with stricter split" << std::endl;
                auto second_split = split_community(chunk, 100, 40);
                for (std::vector<std::vector<int>> &subchunk : second_split)
                {
                    if (!try_build_and_extract(subchunk, witness_clauses))
                    {
                        std::cout << "second build also failed; skipping this subchunk"
                                  << std::endl;
                    }
                }
            }
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