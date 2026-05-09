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