#include "util.hpp"
#include "./bdd/bdd.hpp"
#include "./bdd/bddcreator.hpp"
#include "./varorder/kondylidou_var_order.hpp"

// Prints the node-count breakdown returned by BDD::get_bdd_stats() to stdout.
void print_bdd_stats(BDD &bdd)
{
    bdd_stats stats = bdd.get_bdd_stats();
    std::cout << "BDD stats: num_of_nodes: " << stats.num_of_nodes << std::endl
              << "num_of_default_nodes: " << stats.num_of_default_nodes << std::endl
              << "num_of_sat_nodes: " << stats.num_of_sat_nodes << std::endl
              << "num_of_unsat_nodes: " << stats.num_of_unsat_nodes << std::endl
              << "num_of_infeasible_nodes: " << stats.num_of_infeasible_nodes << std::endl;
}

// Formats a clause as "[l1 l2 ...]" for debug output and log messages.
std::string clause_to_string(std::vector<int>& clause){
    std::string result = "[";

    for(size_t i = 0; i < clause.size(); i++){
        result += std::to_string(clause[i]) + " ";
    }

    result.pop_back();
    result += "]";

    return result;
}

// Maps the NodeStatus enum to its string representation for debug output.
std::string status_to_string(NodeStatus status){
    switch(status){
        case NodeStatus::DEFAULT:{
            return "DEFAULT";
        }
        case NodeStatus::SAT:{
            return "SAT";
        }
        case NodeStatus::UNSAT:{
            return "UNSAT";
        }
        case NodeStatus::INFEASIBLE:{
            return "INFEASIBLE";
        }
        default: {
            return "UNKNOWN TYPE";
        }
    }
}

// Interprets CaDiCaL's result code: 10 = SAT, 20 = UNSAT, anything else = UNKNOWN.
void print_sat_result(int result){
    std::cout << "result: ";
    if(result == 10){
        std::cout << "SAT" << std::endl;
    }else if(result == 20){
        std::cout << "UNSAT" << std::endl;
    }else{
        std::cout << "UNKNOWN" << std::endl;
    }
}

// Prints each witness clause to stdout using clause_to_string format.
void print_witness_clauses(std::vector<std::vector<int>>& clauses){
    for(std::vector<int>& clause : clauses){
        std::cout << clause_to_string(clause) << std::endl;
    }
}


// Splits a community's clause set into chunks that each fit within max_clauses
// and max_vars using greedy fill. A new chunk is opened whenever the next
// clause would violate either cap. A single clause that alone exceeds a cap is
// emitted as its own chunk rather than stalling indefinitely — the BDD builder
// will then throw if it cannot handle it.
std::vector<std::vector<std::vector<int>>> split_community(
    const std::vector<std::vector<int>>& community_clauses,
    std::size_t max_clauses,
    std::size_t max_vars)
{
    // -------------------------------------------------------------------------
    // Base case: community fits in the clause budget. Return as one chunk;
    // do NOT apply the variable cap.
    // -------------------------------------------------------------------------
    //if (community_clauses.size() <= max_clauses) {
    //    return { community_clauses };
    //}
 
    // -------------------------------------------------------------------------
    // Splitting case: enforce both caps with greedy fill.
    // -------------------------------------------------------------------------
    std::vector<std::vector<std::vector<int>>> chunks;
 
    std::vector<std::vector<int>> current;
    std::unordered_set<int>       current_vars;
 
    // Reused per-clause buffer for the variable set of one clause, so we can
    // count NEW variables it would introduce without double-counting literals
    // that appear twice in the same clause (e.g. tautologies x ∨ ¬x).
    std::unordered_set<int> clause_vars;
 
    for (const auto& clause : community_clauses) {
        // Collect the distinct variables of this clause.
        clause_vars.clear();
        for (int lit : clause) {
            const int v = std::abs(lit);
            if (v == 0) continue;
            clause_vars.insert(v);
        }
 
        // Count how many would be NEW relative to the current chunk.
        std::size_t new_vars = 0;
        for (int v : clause_vars) {
            if (current_vars.find(v) == current_vars.end()) ++new_vars;
        }
 
        // Would this clause violate either cap?
        const bool clause_cap_hit = current.size() + 1 > max_clauses;
        const bool var_cap_hit    = current_vars.size() + new_vars > max_vars;
 
        // Close the current chunk if either cap would be violated. Skip the
        // close if the chunk is empty — that means a SINGLE clause alone
        // exceeds a cap (e.g. one clause with > 95 variables); we still emit
        // it as its own chunk to make progress, even though the BDD builder
        // may not be able to handle it. That diagnostic is more useful than
        // an infinite loop.
        if ((clause_cap_hit || var_cap_hit) && !current.empty()) {
            chunks.push_back(std::move(current));
            current = {};
            current_vars.clear();
        }
 
        // Add this clause to the (possibly fresh) current chunk.
        current.push_back(clause);
        current_vars.insert(clause_vars.begin(), clause_vars.end());
    }
 
    // Don't forget the trailing chunk.
    if (!current.empty()) {
        chunks.push_back(std::move(current));
    }
 
    return chunks;
}

// Runs the full per-chunk pipeline: Kondylidou variable ordering → BDD
// construction → witness clause extraction. Appends any produced witness
// clauses to the shared output vector. Returns false (and does not append)
// if BDD construction exceeds MAX_NODES; the caller decides how to retry.
bool try_build_and_extract(
    std::vector<std::vector<int>> &chunk,
    std::vector<std::vector<int>> &witness_clauses,
    int MAX_NODES)
{
    std::cout << "number of clauses in current chunk: " << chunk.size() << std::endl;

    Kondy_VarOrder var_order_creator(chunk);
    std::vector<int> var_order = var_order_creator.create_var_order();
    std::cout << "var order: " << clause_to_string(var_order) << std::endl;
    std::cout << "number of distinct vars in clauses: " << var_order.size() << std::endl;

    try
    {
        BDDCreator creator(chunk, var_order, MAX_NODES);
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

// Parses -i <path>, -n <max_nodes>, and -m <constructive|destructive> from
// argv. Throws std::runtime_error with a descriptive message on any invalid
// or missing argument so main() can print usage and exit cleanly.
ProgramArgs parse_args(int argc, char** argv) {
    ProgramArgs args;

    for (int i = 1; i < argc; ++i) {
        const std::string flag = argv[i];

        if (flag == "-i") {
            if (i + 1 >= argc) {
                throw std::runtime_error("-i requires a path argument");
            }
            args.input_path = argv[++i];
        }
        else if (flag == "-n") {
            if (i + 1 >= argc) {
                throw std::runtime_error("-n requires an integer argument");
            }
            try {
                args.max_nodes = std::stoi(argv[++i]);
            } catch (const std::exception&) {
                throw std::runtime_error("-n argument is not a valid integer: "
                                         + std::string(argv[i]));
            }
            if (args.max_nodes <= 0) {
                throw std::runtime_error("-n argument must be positive");
            }
        }
        else if (flag == "-m") {
            if (i + 1 >= argc) {
                throw std::runtime_error("-m requires a mode argument "
                                         "(constructive | destructive)");
            }
            const std::string value = argv[++i];
            if      (value == "constructive") args.mode = Mode::Constructive;
            else if (value == "destructive")  args.mode = Mode::Destructive;
            else {
                throw std::runtime_error("-m must be 'constructive' or 'destructive', got: "
                                         + value);
            }
        }
        else {
            throw std::runtime_error("unknown argument: " + flag);
        }
    }

    return args;
}