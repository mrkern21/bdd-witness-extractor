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