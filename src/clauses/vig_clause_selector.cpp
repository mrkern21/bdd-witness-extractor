#include "vig_clause_selector.hpp"

// Runs the full VIG+Louvain decomposition eagerly at construction time.
VIG_Clause_Selector::VIG_Clause_Selector(std::vector<std::vector<int>>& clauses) : clauses(clauses)
{
    this->decomp = decompose_cnf(clauses);
    this->community_index = 0;
}

// Returns all clauses assigned to the current community, then advances the
// cursor so the next call returns the following community.
std::vector<std::vector<int>> VIG_Clause_Selector::select_clauses(){
    //print_stats(decomp, std::cout);
    std::vector<std::vector<int>> result;
    for(size_t i = 0; i < clauses.size(); i++){
        if(decomp.clause_to_community[i] == community_index){
            result.push_back(clauses[i]);
        }
    }
    //increment community_index after loop
    community_index++;
    return result;
}

// True while there are communities that select_clauses() has not yet served.
bool VIG_Clause_Selector::has_remaining_community() const {
    return community_index < decomp.num_communities;
}