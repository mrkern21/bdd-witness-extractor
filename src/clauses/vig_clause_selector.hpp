// VIG_Clause_Selector — community-aware clause selection via VIG + Louvain.
// On construction, decomposes the full clause set into communities using the
// Variable Incidence Graph (VIG) and the Louvain algorithm (CommunityDecomposition).
// Each call to select_clauses() returns the clauses belonging to the next
// community in ascending community-id order, advancing an internal cursor.
// Use has_remaining_community() to check whether more communities are available.
// This allows the BDD builder to process one tightly-coupled clause group at a
// time, keeping BDD size manageable.
#ifndef __VIG_CLAUSE_SELECTOR_HPP
#define __VIG_CLAUSE_SELECTOR_HPP

#include "../internal.hpp"
#include "clauseselector.hpp"
#include "../community/community_decomposition.hpp"

class VIG_Clause_Selector : public ClauseSelector{

    private:
        std::vector<std::vector<int>>& clauses;
        int community_index;
        CommunityDecomposition decomp;

    public:
        VIG_Clause_Selector(std::vector<std::vector<int>>& clauses);
        std::vector<std::vector<int>> select_clauses() override;
        bool has_remaining_community() const;

};

#endif