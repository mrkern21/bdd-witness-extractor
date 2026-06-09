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
#include "extraction/extraction_modes.hpp"

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

    // Shared dedup set; both extraction modes and the fast path insert here.
    // Witnesses are canonicalized before insertion so literal-order variants
    // of the same clause are not counted twice.
    std::set<std::vector<int>> unique_witnesses;

    // Build callback shared by both extraction modes.
    BuildAndExtractFn build_fn = [&](std::vector<std::vector<int>>& chunk,
                                     std::vector<std::vector<int>>& out) -> bool {
        return try_build_and_extract(chunk, out, args.max_nodes);
    };

    // Witness-clause extraction: three paths depending on input size and mode.
    //
    // Fast path  (<60 clauses): build one BDD over the whole formula directly.
    //   Community detection on tiny inputs produces poor decompositions and is
    //   not worth the overhead.
    //
    // Destructive path (>=60 clauses): decompose into communities via VIG+Louvain,
    //   then call destructive_extract per community, which recursively halves
    //   the chunk caps on failure until the chunk fits or hits the lower limit.
    //
    // Constructive path (>=60 clauses): same community decomposition, then
    //   call constructive_extract per community, which fine-splits and then
    //   greedily merges chunks by Jaccard variable overlap.
    if (clauses.size() < 60)
    {
        clause_counter += static_cast<int>(clauses.size());
        std::vector<std::vector<int>> extracted;
        if (try_build_and_extract(clauses, extracted, args.max_nodes)) {
            for (auto& w : extracted)
                unique_witnesses.insert(canonicalize_clause(std::move(w)));
        }
    }
    else if (args.mode == Mode::Destructive)
    {
        VIG_Clause_Selector selector(clauses);

        while (selector.has_remaining_community())
        {
            std::vector<std::vector<int>> community = selector.select_clauses();
            std::cout << "number of selected clauses: " << community.size() << std::endl;
            clause_counter += static_cast<int>(community.size());
            destructive_extract(community, unique_witnesses, build_fn);
        }
    }
    else
    {
        VIG_Clause_Selector selector(clauses);

        while (selector.has_remaining_community())
        {
            std::vector<std::vector<int>> community = selector.select_clauses();
            std::cout << "number of selected clauses: " << community.size() << std::endl;
            clause_counter += static_cast<int>(community.size());
            constructive_extract(community, unique_witnesses, build_fn);
        }
    }

    // Flatten the dedup set into a vector for output and CNF writing.
    std::vector<std::vector<int>> witness_clauses(unique_witnesses.begin(),
                                                   unique_witnesses.end());

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