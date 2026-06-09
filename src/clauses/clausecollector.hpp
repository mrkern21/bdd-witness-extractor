// ClauseCollector — harvests all clauses from a CaDiCaL solver instance.
// Implements CaDiCaL::ClauseIterator so it can be passed to
// CaDiCaL::Solver::traverse_clauses(). The clause() callback is invoked once
// per clause by CaDiCaL; after traversal, retrieve_clauses() returns the
// accumulated set, ready to be fed into the BDD construction pipeline.
#ifndef __CLAUSECOLLECTOR_HPP
#define __CLAUSECOLLECTOR_HPP


#include "../internal.hpp"

class ClauseCollector : public CaDiCaL::ClauseIterator{
    private:
        std::vector<std::vector<int>> clauses;

    public:
        bool clause (const std::vector<int> & cl) override;
        const std::vector<std::vector<int>> retrieve_clauses() const;
};

#endif