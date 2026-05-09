// =============================================================================
// community_decomposition.cpp
// -----------------------------------------------------------------------------
// Implementation of decompose_cnf. The algorithm proceeds in six stages:
//
//   0. Discover variables in the input and assign compact 0..n-1 indices
//      (igraph requires consecutive vertex ids).
//   1. Accumulate VIG edge weights into a hash map keyed by variable pairs.
//   2. Convert the hash map into igraph edge / weight vectors and build
//      the graph.
//   3. Run Louvain (igraph_community_multilevel) to obtain a vertex
//      membership vector.
//   4. Translate compact indices back to original variable ids and populate
//      variable_to_community.
//   5. Assign each clause to a community via majority vote of its variables.
//   6. Free igraph resources.
//
// The two igraph error paths (steps 2 and 3) replicate the cleanup inline
// because we are not using RAII. This is slightly repetitive but keeps the
// resource lifetimes visible at every site.
// =============================================================================

#include "community_decomposition.hpp"
#include "../internal.hpp"

namespace {

// -----------------------------------------------------------------------------
// Pair encoding helpers.
//
// We need to use a hash map keyed on unordered variable pairs. std::pair<int,int>
// would work but lacks a built-in hash, and writing one is more code than
// packing the two ints into a 64-bit integer.
//
// Constraints:
//   - pair_key requires a != b (the VIG never has self-loops).
//   - Both ints must fit in 32 bits, which is fine: SAT instances with more
//     than ~4 billion variables do not exist.
// -----------------------------------------------------------------------------

inline std::uint64_t pair_key(int a, int b) {
    if (a > b) std::swap(a, b);  // canonicalize so {a,b} and {b,a} map to the same key
    return (static_cast<std::uint64_t>(static_cast<std::uint32_t>(a)) << 32)
         |  static_cast<std::uint64_t>(static_cast<std::uint32_t>(b));
}

inline std::pair<int, int> pair_decode(std::uint64_t k) {
    return { static_cast<int>(k >> 32),
             static_cast<int>(k & 0xFFFFFFFFull) };
}

} // namespace

CommunityDecomposition decompose_cnf(
    const std::vector<std::vector<int>>& clauses)
{
    // -------------------------------------------------------------------------
    // 0. Discover variables and assign compact indices.
    //
    // Original variable ids may be sparse and not start at 1 (e.g. when
    // operating on a subset of a parent CNF). igraph, however, requires
    // vertex ids in the range [0, n). We build a bidirectional mapping:
    //   var_to_index : original id -> compact index [0, n)
    //   index_to_var : compact index -> original id
    // The compact indices are used internally for igraph; the original ids
    // are used in the output struct so the caller never has to translate.
    // -------------------------------------------------------------------------
    std::unordered_map<int, int> var_to_index;
    std::vector<int>             index_to_var;
    var_to_index.reserve(clauses.size() * 4);  // heuristic preallocation

    for (const auto& clause : clauses) {
        for (int lit : clause) {
            const int v = std::abs(lit);
            if (v == 0) continue;  // 0 is the DIMACS clause terminator; skip defensively

            // try_emplace inserts only if the key is missing. The new value
            // is the next available compact index, which we then push onto
            // index_to_var to keep the inverse map in sync.
            auto [it, inserted] = var_to_index.try_emplace(
                v, static_cast<int>(index_to_var.size()));
            if (inserted) index_to_var.push_back(v);
        }
    }
    const int num_vars = static_cast<int>(index_to_var.size());

    // Defensive: empty input or formula with only empty clauses.
    if (num_vars == 0) {
        CommunityDecomposition empty;
        empty.clause_to_community.assign(clauses.size(), -1);
        return empty;
    }

    // -------------------------------------------------------------------------
    // 1. Accumulate VIG edge weights.
    //
    // For each clause with k >= 2 distinct variables, every unordered pair
    // of those variables receives weight 1 / C(k, 2) = 2 / (k * (k - 1)).
    // The same pair may appear in multiple clauses; weights accumulate.
    //
    // Unit clauses (k=1) and empty clauses (k=0) contribute no edges, since
    // they involve no pair of variables. They will not influence the VIG;
    // their variables (if any) still appear in the graph through other
    // clauses they participate in.
    //
    // Implementation note: we deduplicate literals within a clause via an
    // unordered_set, so a malformed clause like (x ∨ x ∨ ¬y) is treated as
    // {x, y} — a 2-variable clause — rather than producing duplicate edges.
    // Tautological clauses (containing both x and ¬x) are NOT detected here;
    // they would be handled by a preprocessor like SatELite if used.
    // -------------------------------------------------------------------------
    std::unordered_map<std::uint64_t, double> edge_weight;
    edge_weight.reserve(clauses.size() * 4);

    std::unordered_set<int> distinct_idx;  // reused across clauses
    std::vector<int>        idx_list;      // reused across clauses

    for (const auto& clause : clauses) {
        // Collect the distinct compact indices of variables in this clause.
        distinct_idx.clear();
        for (int lit : clause) {
            const int v = std::abs(lit);
            if (v == 0) continue;
            distinct_idx.insert(var_to_index[v]);
        }

        const int k = static_cast<int>(distinct_idx.size());
        if (k < 2) continue;  // unit/empty clauses contribute no edges

        // Per-clause edge weight, distributed equally over all C(k, 2) pairs.
        const double pair_count = 0.5 * k * (k - 1);
        const double w          = 1.0 / pair_count;

        // Enumerate the C(k, 2) pairs and add w to each.
        idx_list.assign(distinct_idx.begin(), distinct_idx.end());
        for (int i = 0; i < k; ++i) {
            for (int j = i + 1; j < k; ++j) {
                edge_weight[pair_key(idx_list[i], idx_list[j])] += w;
            }
        }
    }

    // -------------------------------------------------------------------------
    // 2. Build the igraph graph.
    //
    // igraph_create takes a flat edge vector of length 2 * |E|, where pairs
    // (edges[2i], edges[2i+1]) are the endpoints of edge i. The weights are
    // passed separately to the community algorithm.
    // -------------------------------------------------------------------------
    igraph_vector_int_t edges;
    igraph_vector_t     weights;
    igraph_vector_int_init(&edges,   2 * static_cast<igraph_integer_t>(edge_weight.size()));
    igraph_vector_init    (&weights,     static_cast<igraph_integer_t>(edge_weight.size()));

    {
        igraph_integer_t i = 0;
        for (const auto& kv : edge_weight) {
            auto [a, b] = pair_decode(kv.first);
            VECTOR(edges)[2 * i]     = a;       // already a 0-based compact index
            VECTOR(edges)[2 * i + 1] = b;
            VECTOR(weights)[i]       = kv.second;
            ++i;
        }
    }

    igraph_t graph;
    if (igraph_create(&graph, &edges, num_vars, IGRAPH_UNDIRECTED) != IGRAPH_SUCCESS) {
        // Cleanup before throwing: edges and weights are the only resources
        // alive at this point.
        igraph_vector_int_destroy(&edges);
        igraph_vector_destroy(&weights);
        throw std::runtime_error("igraph_create failed");
    }

    // -------------------------------------------------------------------------
    // 3. Run Louvain.
    //
    // igraph_community_multilevel implements the Louvain algorithm. It
    // performs a hierarchical agglomeration; we only consume the final
    // (highest-modularity) level via `membership`.
    //
    // The `modularities` vector receives the modularity value at every
    // hierarchy level — useful to inspect the algorithm's progression. We
    // only keep the last one for the output struct.
    //
    // Resolution = 1.0 is the standard Louvain setting (matches Newman's
    // original modularity definition). Higher values bias toward smaller
    // communities; lower values toward fewer, larger ones.
    //
    // Determinism caveat: Louvain's vertex traversal order is randomized.
    // For reproducibility, seed igraph's RNG once at program start with:
    //     igraph_rng_seed(igraph_rng_default(), <some seed>);
    // -------------------------------------------------------------------------
    igraph_vector_int_t membership;
    igraph_vector_t     modularities;
    igraph_vector_int_init(&membership,   0);
    igraph_vector_init    (&modularities, 0);

    if (igraph_community_multilevel(
            &graph,
            &weights,
            /*resolution=*/ 1.0,
            &membership,
            /*memberships=*/ nullptr,  // we don't need every hierarchy level
            &modularities) != IGRAPH_SUCCESS)
    {
        igraph_vector_int_destroy(&membership);
        igraph_vector_destroy(&modularities);
        igraph_destroy(&graph);
        igraph_vector_int_destroy(&edges);
        igraph_vector_destroy(&weights);
        throw std::runtime_error("igraph_community_multilevel failed");
    }

    // -------------------------------------------------------------------------
    // 4. Read variable -> community using ORIGINAL ids.
    //
    // membership[idx] gives the community of compact index idx; we look up
    // the original variable id via index_to_var[idx] before storing.
    // -------------------------------------------------------------------------
    CommunityDecomposition out;

    // Final modularity = last entry of the hierarchy-level vector.
    out.modularity =
        igraph_vector_size(&modularities) > 0
            ? VECTOR(modularities)[igraph_vector_size(&modularities) - 1]
            : 0.0;

    out.variable_to_community.reserve(num_vars);
    int max_comm = -1;
    for (int idx = 0; idx < num_vars; ++idx) {
        const int c = static_cast<int>(VECTOR(membership)[idx]);
        out.variable_to_community[index_to_var[idx]] = c;
        if (c > max_comm) max_comm = c;
    }
    // Community ids range from 0 to max_comm; total count is max_comm + 1.
    out.num_communities = max_comm + 1;

    // -------------------------------------------------------------------------
    // 5. Majority-vote clause assignment.
    //
    // Each clause is assigned to the community holding the plurality of its
    // variables. Ties are broken by smallest community id (deterministic).
    //
    // We use a `tally` buffer of size num_communities reused across clauses,
    // and a `touched` list of just the community ids we've incremented in
    // the current clause — this lets us reset only those entries to zero
    // afterwards instead of wiping the whole tally vector each time. This
    // is important when num_communities is large but clauses are short.
    // -------------------------------------------------------------------------
    out.clause_to_community.assign(clauses.size(), -1);
    out.community_variables.assign(out.num_communities, {});
    out.community_clauses.assign  (out.num_communities, {});

    // Populate the inverse variables map.
    for (const auto& kv : out.variable_to_community) {
        out.community_variables[kv.second].push_back(kv.first);
    }

    std::vector<int> tally(out.num_communities, 0);
    std::vector<int> touched;
    touched.reserve(32);  // most clauses touch only a handful of communities

    for (std::size_t ci = 0; ci < clauses.size(); ++ci) {
        // Tally how many variables of this clause fall into each community.
        touched.clear();
        for (int lit : clauses[ci]) {
            const int v = std::abs(lit);
            if (v == 0) continue;

            // var_to_index is guaranteed to contain v: we built it from
            // exactly these clauses in step 0.
            const int idx = var_to_index[v];
            const int c   = static_cast<int>(VECTOR(membership)[idx]);

            if (tally[c] == 0) touched.push_back(c);
            ++tally[c];
        }

        // Find the community with the highest count. Ties favor smaller id.
        int best = -1, best_count = -1;
        for (int c : touched) {
            const int t = tally[c];
            if (t > best_count || (t == best_count && c < best)) {
                best       = c;
                best_count = t;
            }
        }

        // Reset the tally entries we touched (cheaper than wiping all).
        for (int c : touched) tally[c] = 0;

        if (best >= 0) {
            out.clause_to_community[ci] = best;
            out.community_clauses[best].push_back(ci);
        }
        // best == -1 only when the clause is empty (no variables to vote);
        // clause_to_community stays at the -1 sentinel set above.
    }

    // -------------------------------------------------------------------------
    // 6. Clean up igraph resources.
    //
    // Destroy in reverse order of creation (not strictly required by igraph,
    // but it mirrors the pairing visually).
    // -------------------------------------------------------------------------
    igraph_vector_int_destroy(&membership);
    igraph_vector_destroy(&modularities);
    igraph_destroy(&graph);
    igraph_vector_int_destroy(&edges);
    igraph_vector_destroy(&weights);

    return out;
}

void print_stats(
    const CommunityDecomposition& d,
    std::ostream& os)
{
    const std::size_t total_vars    = d.variable_to_community.size();
    const std::size_t total_clauses = d.clause_to_community.size();
 
    // Count clauses that did not get assigned to any community (only happens
    // for empty clauses in practice).
    std::size_t unassigned = 0;
    for (int c : d.clause_to_community) if (c < 0) ++unassigned;
 
    os << "=== CommunityDecomposition stats ===\n";
    os << "  modularity Q       : " << d.modularity     << "\n";
    os << "  communities        : " << d.num_communities << "\n";
    os << "  variables          : " << total_vars       << "\n";
    os << "  clauses            : " << total_clauses    << "\n";
    os << "  unassigned clauses : " << unassigned       << "\n";
}


// =============================================================================
// print_clause_assignments
// -----------------------------------------------------------------------------
// One line per clause: index in the input vector + assigned community id.
// Unassigned clauses (community id -1) are printed with "unassigned" so the
// caller can spot empty / malformed clauses.
// =============================================================================
void print_clause_assignments(
    const CommunityDecomposition& d,
    std::ostream& os)
{
    os << "=== clause -> community ===\n";
    for (std::size_t i = 0; i < d.clause_to_community.size(); ++i) {
        const int c = d.clause_to_community[i];
        os << "  clause " << i << " -> ";
        if (c < 0) os << "unassigned\n";
        else       os << "community " << c << "\n";
    }
}