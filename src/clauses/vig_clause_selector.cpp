#include "vig_clause_selector.hpp"

VIG_Clause_Selector::VIG_Clause_Selector(std::vector<std::vector<int>>& clauses) : clauses(clauses)
{
    init_communities();
    this->community_index = 0;
}

void VIG_Clause_Selector::init_communities()
{
    decomp = decompose_cnf(clauses);
}

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

bool VIG_Clause_Selector::has_remaining_community() const {
    return community_index < decomp.num_communities;
}