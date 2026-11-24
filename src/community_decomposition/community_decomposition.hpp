// community_decomposition.hpp — decomposes a CNF formula into communities via
// the Variable Incidence Graph (VIG) and the Louvain algorithm.
// Each variable becomes a VIG vertex; each clause of size k contributes weight
// 1/C(k,2) to every variable pair it contains so all clauses contribute equal
// total weight regardless of length. Louvain then partitions variables into
// communities maximising modularity (Ansótegui, Giráldez-Cru, Levy, SAT 2012).
// Each clause is assigned to the community holding the plurality of its variables;
// ties are broken by smallest community id for determinism.
#ifndef __COMMUNITY_DECOMP_HPP
#define __COMMUNITY_DECOMP_HPP

#include <igraph.h>

#include <cstddef>
#include <cmath>
#include <cstdint>
#include <cstdlib>
#include <stdexcept>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>

// Result of decomposing a CNF formula into communities.
// Community ids form a contiguous range [0, num_communities).
struct CommunityDecomposition {
    // Total number of communities found by Louvain.
    int num_communities = 0;

    // Maps original variable id -> community id.
    // Contains exactly the variables that appear in the input clauses.
    std::unordered_map<int, int> variable_to_community;

    // Parallel to the input clauses vector: clause i's assigned community id.
    std::vector<int> clause_to_community;

    // Inverse maps: community_variables[c] lists the variable ids in community c;
    // community_clauses[c] lists the indices into the input clauses vector.
    std::vector<std::vector<int>>         community_variables;
    std::vector<std::vector<std::size_t>> community_clauses;

    // Modularity Q ∈ [-1/2, 1] of the final Louvain partition.
    // Industrial SAT instances typically yield Q ≈ 0.7–0.9;
    // random 3-SAT near the phase transition yields Q ≈ 0.15.
    double modularity = 0.0;
};

// Decomposes a CNF formula in DIMACS literal convention into communities.
// Variable ids may be sparse and need not start at 1.
// Throws std::runtime_error if any underlying igraph call fails.
CommunityDecomposition decompose_cnf(
    const std::vector<std::vector<int>>& clauses);

// Prints a compact summary of a CommunityDecomposition to os:
// modularity, community count, variable count, clause count, unassigned clauses.
void print_stats(
    const CommunityDecomposition& d,
    std::ostream& os);

// Prints one line per clause showing its index and assigned community id.
// Produces one line per clause; use print_stats for a compact overview instead.
void print_clause_assignments(
    const CommunityDecomposition& d,
    std::ostream& os);

#endif
