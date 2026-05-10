#include "util.hpp"
#include "./bdd/bdd.hpp"

void print_bdd_stats(BDD &bdd)
{
    bdd_stats stats = bdd.get_bdd_stats();
    std::cout << "BDD stats: num_of_nodes: " << stats.num_of_nodes << std::endl
              << "num_of_default_nodes: " << stats.num_of_default_nodes << std::endl
              << "num_of_sat_nodes: " << stats.num_of_sat_nodes << std::endl
              << "num_of_unsat_nodes: " << stats.num_of_unsat_nodes << std::endl
              << "num_of_infeasible_nodes: " << stats.num_of_infeasible_nodes << std::endl;
}

std::string clause_to_string(std::vector<int>& clause){
    std::string result = "[";

    for(size_t i = 0; i < clause.size(); i++){
        result += std::to_string(clause[i]) + " ";
    }

    result.pop_back();
    result += "]";

    return result;
}

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

void print_witness_clauses(std::vector<std::vector<int>>& clauses){
    for(std::vector<int>& clause : clauses){
        std::cout << clause_to_string(clause) << std::endl;
    }
}


std::vector<std::vector<std::vector<int>>> split_community(
    const std::vector<std::vector<int>>& community_clauses,
    std::size_t max_clauses,
    std::size_t max_vars)
{
    // -------------------------------------------------------------------------
    // Base case: community fits in the clause budget. Return as one chunk;
    // do NOT apply the variable cap.
    // -------------------------------------------------------------------------
    if (community_clauses.size() <= max_clauses) {
        return { community_clauses };
    }
 
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