// decompose_cnf proceeds in seven stages (0-6):
//   0. Discover variables and assign compact 0..n-1 indices (igraph requires
//      consecutive vertex ids; original ids may be sparse).
//   1. Accumulate VIG edge weights into a hash map keyed by variable pairs.
//   2. Convert the hash map into igraph edge/weight vectors and build the graph.
//   3. Run Louvain to obtain a vertex membership vector.
//   4. Translate compact indices back to original variable ids.
//   5. Assign each clause to a community via majority vote of its variables.
//   6. Free igraph resources.
// The two igraph error paths (steps 2 and 3) replicate cleanup inline because
// RAII is not used; this keeps resource lifetimes visible at every error site.

#include "community_decomposition.hpp"
#include "../internal.hpp"

namespace {

// Encodes an unordered variable pair {a, b} into a single uint64 key so the
// edge-weight map needs no custom hash. Packing into 64 bits is safe because
// SAT instances with more than ~4 billion variables do not exist.
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
    // Stage 0: build a bidirectional mapping between original variable ids and
    // compact 0-based indices. igraph requires vertex ids in [0, n); original
    // ids are used in the output struct so the caller never has to translate.
    std::unordered_map<int, int> var_to_index;
    std::vector<int>             index_to_var;
    var_to_index.reserve(clauses.size() * 4);

    for (const auto& clause : clauses) {
        for (int lit : clause) {
            const int v = std::abs(lit);
            if (v == 0) continue;  // 0 is the DIMACS clause terminator; skip defensively

            // try_emplace inserts only if the key is missing; the new value is
            // the next compact index, kept in sync with index_to_var.
            auto [it, inserted] = var_to_index.try_emplace(
                v, static_cast<int>(index_to_var.size()));
            if (inserted) index_to_var.push_back(v);
        }
    }
    const int num_vars = static_cast<int>(index_to_var.size());

    // Empty input or formula with only empty clauses — nothing to decompose.
    if (num_vars == 0) {
        CommunityDecomposition empty;
        empty.clause_to_community.assign(clauses.size(), -1);
        return empty;
    }

    // Stage 1: accumulate VIG edge weights.
    // Each clause of k >= 2 distinct variables distributes weight 1/C(k,2)
    // across all its variable pairs so every clause contributes total weight 1.
    // Literals within a clause are deduplicated via distinct_idx so malformed
    // clauses like (x ∨ x ∨ ¬y) don't inflate edge weights.
    std::unordered_map<std::uint64_t, double> edge_weight;
    edge_weight.reserve(clauses.size() * 4);

    std::unordered_set<int> distinct_idx;  // reused across clauses
    std::vector<int>        idx_list;      // reused across clauses

    for (const auto& clause : clauses) {
        distinct_idx.clear();
        for (int lit : clause) {
            const int v = std::abs(lit);
            if (v == 0) continue;
            distinct_idx.insert(var_to_index[v]);
        }

        const int k = static_cast<int>(distinct_idx.size());
        if (k < 2) continue;  // unit/empty clauses contribute no edges

        const double pair_count = 0.5 * k * (k - 1);
        const double w          = 1.0 / pair_count;

        idx_list.assign(distinct_idx.begin(), distinct_idx.end());
        for (int i = 0; i < k; ++i) {
            for (int j = i + 1; j < k; ++j) {
                edge_weight[pair_key(idx_list[i], idx_list[j])] += w;
            }
        }
    }

    // Stage 2: build the igraph undirected weighted graph.
    // igraph_create expects a flat edge vector where (edges[2i], edges[2i+1])
    // are the endpoints of edge i; weights are passed separately to Louvain.
    igraph_vector_int_t edges;
    igraph_vector_t     weights;
    igraph_vector_int_init(&edges,   2 * static_cast<igraph_integer_t>(edge_weight.size()));
    igraph_vector_init    (&weights,     static_cast<igraph_integer_t>(edge_weight.size()));

    {
        igraph_integer_t i = 0;
        for (const auto& kv : edge_weight) {
            auto [a, b] = pair_decode(kv.first);
            VECTOR(edges)[2 * i]     = a;
            VECTOR(edges)[2 * i + 1] = b;
            VECTOR(weights)[i]       = kv.second;
            ++i;
        }
    }

    igraph_t graph;
    if (igraph_create(&graph, &edges, num_vars, IGRAPH_UNDIRECTED) != IGRAPH_SUCCESS) {
        igraph_vector_int_destroy(&edges);
        igraph_vector_destroy(&weights);
        throw std::runtime_error("igraph_create failed");
    }

    // Stage 3: run Louvain. We only consume the final (highest-modularity)
    // hierarchy level via membership; intermediate levels are discarded.
    // Resolution = 1.0 matches Newman's original modularity definition.
    // Louvain's traversal order is randomized — seed igraph's RNG for
    // reproducibility: igraph_rng_seed(igraph_rng_default(), <seed>).
    igraph_vector_int_t membership;
    igraph_vector_t     modularities;
    igraph_vector_int_init(&membership,   0);
    igraph_vector_init    (&modularities, 0);

    if (igraph_community_multilevel(
            &graph,
            &weights,
            /*resolution=*/ 1.0,
            &membership,
            /*memberships=*/ nullptr,  // intermediate levels not needed
            &modularities) != IGRAPH_SUCCESS)
    {
        igraph_vector_int_destroy(&membership);
        igraph_vector_destroy(&modularities);
        igraph_destroy(&graph);
        igraph_vector_int_destroy(&edges);
        igraph_vector_destroy(&weights);
        throw std::runtime_error("igraph_community_multilevel failed");
    }

    // Stage 4: translate compact indices back to original variable ids.
    CommunityDecomposition out;

    // Final modularity is the last entry of the hierarchy-level vector.
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
    out.num_communities = max_comm + 1;

    // Stage 5: majority-vote clause assignment.
    // tally is reused across clauses; touched records only the community ids
    // incremented for the current clause so we can zero just those entries
    // rather than clearing the whole tally each time — important when
    // num_communities is large but individual clauses are short.
    out.clause_to_community.assign(clauses.size(), -1);
    out.community_variables.assign(out.num_communities, {});
    out.community_clauses.assign  (out.num_communities, {});

    for (const auto& kv : out.variable_to_community) {
        out.community_variables[kv.second].push_back(kv.first);
    }

    std::vector<int> tally(out.num_communities, 0);
    std::vector<int> touched;
    touched.reserve(32);

    for (std::size_t ci = 0; ci < clauses.size(); ++ci) {
        touched.clear();
        for (int lit : clauses[ci]) {
            const int v = std::abs(lit);
            if (v == 0) continue;

            const int idx = var_to_index[v];
            const int c   = static_cast<int>(VECTOR(membership)[idx]);

            if (tally[c] == 0) touched.push_back(c);
            ++tally[c];
        }

        // Plurality winner; ties broken by smallest community id.
        int best = -1, best_count = -1;
        for (int c : touched) {
            const int t = tally[c];
            if (t > best_count || (t == best_count && c < best)) {
                best       = c;
                best_count = t;
            }
        }

        for (int c : touched) tally[c] = 0;

        if (best >= 0) {
            out.clause_to_community[ci] = best;
            out.community_clauses[best].push_back(ci);
        }
        // best == -1 only for empty clauses; clause_to_community stays -1.
    }

    // Stage 6: free igraph resources in reverse creation order.
    igraph_vector_int_destroy(&membership);
    igraph_vector_destroy(&modularities);
    igraph_destroy(&graph);
    igraph_vector_int_destroy(&edges);
    igraph_vector_destroy(&weights);

    return out;
}

// Counts unassigned clauses (community id -1, i.e. empty clauses in practice)
// and prints a one-line summary per field.
void print_stats(
    const CommunityDecomposition& d,
    std::ostream& os)
{
    const std::size_t total_vars    = d.variable_to_community.size();
    const std::size_t total_clauses = d.clause_to_community.size();

    std::size_t unassigned = 0;
    for (int c : d.clause_to_community) if (c < 0) ++unassigned;

    os << "modularity Q       : " << d.modularity     << "\n";
    os << "communities        : " << d.num_communities << "\n";
    os << "variables          : " << total_vars       << "\n";
    os << "clauses            : " << total_clauses    << "\n";
    os << "unassigned clauses : " << unassigned       << "\n";
}

// Prints one line per clause; unassigned clauses (empty inputs) show
// "unassigned" so the caller can spot malformed entries.
void print_clause_assignments(
    const CommunityDecomposition& d,
    std::ostream& os)
{
    for (std::size_t i = 0; i < d.clause_to_community.size(); ++i) {
        const int c = d.clause_to_community[i];
        os << "  clause " << i << " -> ";
        if (c < 0) os << "unassigned\n";
        else       os << "community " << c << "\n";
    }
}
