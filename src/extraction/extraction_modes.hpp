// extraction_modes.hpp — witness-clause extraction strategies.
// Provides two modes that operate on a single community's clause set and
// accumulate canonicalized witness clauses into a shared dedup set:
//
//   destructive_extract  — splits the community into chunks and builds a BDD
//                          per chunk. On failure, halves the caps and recurses
//                          on sub-chunks until construction succeeds or the
//                          chunk falls to or below lower_limit_clauses.
//
//   constructive_extract — fine-splits the community, builds each piece, then
//                          greedily merges pairs of chunks by variable-set
//                          Jaccard overlap until k consecutive merges fail or
//                          no eligible pairs remain.
//
// Both take the same build callback so BDD construction logic is not repeated
// at the call site.
#ifndef __EXTRACTION_MODES_HPP
#define __EXTRACTION_MODES_HPP

#include "../internal.hpp"
#include <cstddef>
#include <functional>
#include <set>

// Callback for building a BDD from a clause chunk and collecting its witness
// clauses. Returns true on success (witnesses appended to out), false when
// construction exceeds the node limit.
using BuildAndExtractFn = std::function<bool(
    std::vector<std::vector<int>>&,
    std::vector<std::vector<int>>&)>;

// Sorts and deduplicates literals within a clause so that witnesses with
// differently-ordered literals compare equal in the deduplication set.
std::vector<int> canonicalize_clause(std::vector<int> clause);

// Destructive extraction: splits community_clauses at the initial caps, builds
// a BDD per chunk. On failure, recursively halves the caps and retries on
// sub-chunks until either construction succeeds or the chunk size falls to or
// below lower_limit_clauses (below that, further splitting is unlikely to help).
void destructive_extract(
    const std::vector<std::vector<int>>& community_clauses,
    std::set<std::vector<int>>& unique_witnesses,
    BuildAndExtractFn build_and_extract,
    std::size_t initial_max_clauses = 300,
    std::size_t initial_max_vars    = 95,
    std::size_t lower_limit_clauses = 20);

// Constructive extraction: fine-splits the community, builds each piece, then
// greedily merges chunk pairs by variable-set Jaccard overlap to produce larger
// BDDs and more general witness clauses. Stops after k_max_consecutive_failures
// failed merge attempts or when no eligible pairs remain.
void constructive_extract(
    const std::vector<std::vector<int>>& community_clauses,
    std::set<std::vector<int>>& unique_witnesses,
    BuildAndExtractFn build_and_extract,
    int         k_max_consecutive_failures = 5,
    std::size_t initial_max_clauses        = 60,
    std::size_t initial_max_vars           = 30);

#endif
