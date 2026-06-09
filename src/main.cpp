#include "internal.hpp"
#include "bdd/bddcreator.hpp"
#include "clauses/clausecollector.hpp"
#include "varorder/simple_varorder.hpp"
#include "varorder/kondylidou_var_order.hpp"
#include "clauses/simple_clause_selector.hpp"
#include "clauses/vig_clause_selector.hpp"
#include "bdd/bdd.hpp"
#include "cnf_writer/cnf_writer.hpp"
#include "util.hpp"

int main(int argc, char *argv[])
{
    ProgramArgs args;
    try
    {
        args = parse_args(argc, argv);
    }
    catch (const std::exception &e)
    {
        std::cerr << "argument error: " << e.what() << "\n"
                  << "usage: " << argv[0]
                  << " [-i <path>] [-n <max_nodes>] [-m constructive|destructive]\n";
        return 1;
    }

    std::cout << "input file : " << args.input_path << "\n";
    std::cout << "max nodes  : " << args.max_nodes << "\n";
    std::cout << "mode       : "
              << (args.mode == Mode::Constructive ? "constructive" : "destructive")
              << "\n";

    // Load the CNF via CaDiCaL's parser, then harvest all clauses through the
    // ClauseIterator interface. CaDiCaL normalizes the input (e.g. removes
    // tautologies), so the collected clauses may differ slightly from the raw file.
    const char *path = args.input_path.c_str();
    int num_of_vars;

    CaDiCaL::Solver solver;
    ClauseCollector collector;

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

    // Witness-clause extraction: three paths depending on input size and mode.
    //
    // Fast path  (<60 clauses): build one BDD over the whole formula directly.
    //   Community detection on tiny inputs produces poor decompositions and is
    //   not worth the overhead.
    //
    // Destructive path (>=60 clauses): decompose into communities via VIG+Louvain,
    //   split each community into BDD-sized chunks, and extract witnesses per
    //   chunk. On first-attempt failure (MAX_NODES exceeded), retry with a
    //   stricter split (100 clauses / 40 vars); sub-chunks that still fail are
    //   skipped so one hard community does not block the rest.
    //
    // Constructive path: not yet implemented (placeholder else branch).
    if (clauses.size() < 60)
    {
        clause_counter += static_cast<int>(clauses.size());
        try_build_and_extract(clauses, witness_clauses, args.max_nodes);
    }
    else if (args.mode == Mode::Destructive)
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
                if (try_build_and_extract(chunk, witness_clauses, args.max_nodes))
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
                    if (!try_build_and_extract(subchunk, witness_clauses, args.max_nodes))
                    {
                        std::cout << "second build also failed; skipping this subchunk"
                                  << std::endl;
                    }
                }
            }
        }
    }
    else
    {
        
    }

    std::cout << std::endl;
    std::cout << "number of total witness clauses: " << witness_clauses.size() << std::endl;
    std::cout << "number of inspected clauses: " << clause_counter << std::endl;

    // Derive the output path by inserting ".augmented" before the last
    // extension (e.g. "foo.cnf" -> "foo.augmented.cnf"). If there is no
    // extension, the suffix is appended directly.
    std::string output_path = args.input_path;
    const auto dot = output_path.find_last_of('.');
    if (dot != std::string::npos)
    {
        output_path.insert(dot, ".augmented");
    }
    else
    {
        output_path += ".augmented";
    }

    try
    {
        write_augmented_cnf(args.input_path, witness_clauses, output_path);
        std::cout << "wrote augmented CNF to " << output_path
                  << " (" << witness_clauses.size() << " witness clauses appended)\n";
    }
    catch (const std::exception &e)
    {
        std::cerr << "failed to write augmented CNF: " << e.what() << "\n";
        return 1;
    }

    return 0;
}