#ifndef __COMMUNITY_DECOMP_HPP
#define __COMMUNITY_DECOMP_HPP

// =============================================================================
// community_decomposition.hpp
// -----------------------------------------------------------------------------
// Decomposes a CNF SAT formula into communities via the Variable Incidence
// Graph (VIG) and the Louvain algorithm (igraph_community_multilevel).
//
// Background
// ----------
// Industrial SAT instances exhibit strong community structure: their variables
// cluster into groups that are densely interconnected within a group and only
// loosely connected across groups. This is the structural property analyzed in
//   Ansótegui, Giráldez-Cru, Levy. "The Community Structure of SAT Formulas."
//   SAT 2012.
//
// The VIG model represents a CNF formula as a weighted undirected graph in
// which:
//   - vertices are the variables that appear in the formula,
//   - each clause of size k contributes weight 1 / C(k, 2) to every unordered
//     pair of variables it contains. This normalization ensures every clause
//     contributes a total edge weight of 1 to the graph, so long clauses do
//     not dominate short ones.
//
// Louvain (Blondel et al. 2008) is then run to partition the variables into
// communities maximizing modularity (Newman & Girvan 2004). After Louvain
// returns, each clause is assigned to the community holding the plurality of
// its variables; ties are broken by smallest community id (deterministic).
//
// Note that unit clauses (k=1) and empty clauses (k=0) contribute no edges
// to the VIG, since they involve no pair of variables.
// =============================================================================
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
//
// Community ids form a contiguous range [0, num_communities). A community id
// of -1 in `clause_to_community` means the clause could not be assigned to
// any community (in practice, only empty clauses).
struct CommunityDecomposition {
    // Total number of communities found by Louvain.
    int num_communities = 0;

    // Maps original variable id -> community id.
    // Contains exactly the variables that appear in the input clauses.
    // (Original ids are the ints used in the input, e.g. variable 42.)
    std::unordered_map<int, int> variable_to_community;

    // Parallel to the input `clauses` vector: clause i's community id.
    std::vector<int> clause_to_community;

    // Inverse maps for convenient iteration.
    // community_variables[c] = original variable ids assigned to community c.
    // community_clauses[c]   = indices into the input `clauses` vector of
    //                          clauses assigned to community c.
    std::vector<std::vector<int>>         community_variables;
    std::vector<std::vector<std::size_t>> community_clauses;

    // Modularity Q ∈ [-1/2, 1] of the final partition (last hierarchy level
    // of Louvain). Industrial SAT instances typically yield Q ≈ 0.7–0.9;
    // random 3-SAT near the phase transition yields Q ≈ 0.15.
    double modularity = 0.0;
};

// Decompose a CNF formula into communities.
//
// Input:
//   `clauses`: vector of clauses in DIMACS literal convention. Each int is a
//   literal, the sign denotes polarity, and the absolute value is the
//   variable id. Variable ids may be any positive integers, sparse, and need
//   not start at 1 (useful when working on subsets of a parent formula).
//   The literal 0 is treated as invalid and silently skipped (DIMACS uses 0
//   as a clause terminator, but it should not appear inside a clause vector).
//
// Output:
//   A CommunityDecomposition with the partition and modularity value.
//
// Throws std::runtime_error if any underlying igraph call fails.
CommunityDecomposition decompose_cnf(
    const std::vector<std::vector<int>>& clauses);

// Print a human-readable summary of a CommunityDecomposition to `os`.
// Reports modularity, totals, unassigned clauses
void print_stats(
    const CommunityDecomposition& d,
    std::ostream& os);
 
// Print one line per clause, listing its index and assigned community id.
// Format: "  clause <index> -> community <id>"
// Note: produces one line per clause in the input; on industrial instances
// this can be very long. Use print_stats for a compact overview instead.
void print_clause_assignments(
    const CommunityDecomposition& d,
    std::ostream& os);

#endif