#include "extraction_modes.hpp"
#include "../util.hpp"
#include <set>
#include <utility>

// Sorts literals and removes duplicates so that two representations of the
// same clause always produce identical keys in the dedup set.
std::vector<int> canonicalize_clause(std::vector<int> clause) {
    std::sort(clause.begin(), clause.end());
    clause.erase(std::unique(clause.begin(), clause.end()), clause.end());
    return clause;
}

namespace {

// Internal chunk representation for constructive mode: clause set plus the
// distinct variable footprint, kept in sync so Jaccard is O(min(|A|,|B|))
// rather than requiring a full set rebuild on every comparison.
struct Chunk {
    std::vector<std::vector<int>> clauses;
    std::unordered_set<int>       variables;
};

Chunk make_chunk(std::vector<std::vector<int>> clauses) {
    Chunk c;
    c.clauses = std::move(clauses);
    for (const auto& clause : c.clauses) {
        for (int lit : clause) {
            if (lit != 0) c.variables.insert(std::abs(lit));
        }
    }
    return c;
}

// Jaccard similarity |A ∩ B| / |A ∪ B| between two variable sets.
// Iterates the smaller set to minimise the number of hash lookups.
double jaccard(const std::unordered_set<int>& a,
               const std::unordered_set<int>& b)
{
    if (a.empty() && b.empty()) return 0.0;
    const auto& smaller = a.size() < b.size() ? a : b;
    const auto& larger  = a.size() < b.size() ? b : a;
    int inter = 0;
    for (int v : smaller) if (larger.count(v)) ++inter;
    const int uni = static_cast<int>(a.size() + b.size()) - inter;
    return uni == 0 ? 0.0 : static_cast<double>(inter) / uni;
}

// Canonical unordered-pair key used to record failed merge attempts so the
// same pair is never retried.
inline std::pair<int, int> make_pair_key(int a, int b) {
    return { std::min(a, b), std::max(a, b) };
}

// Canonicalizes and inserts all extracted witnesses into the shared dedup set.
void absorb_witnesses(std::vector<std::vector<int>>& extracted,
                      std::set<std::vector<int>>& sink)
{
    for (auto& w : extracted) {
        sink.insert(canonicalize_clause(std::move(w)));
    }
}

// Recursive helper for destructive_extract. Caps are halved at each recursion
// level, guaranteeing termination: split_community produces strictly smaller
// chunks whenever a cap is exceeded, so chunk size decreases monotonically
// until it falls at or below lower_limit_clauses.
void destructive_extract_chunk(
    std::vector<std::vector<int>>& chunk,
    std::set<std::vector<int>>& unique_witnesses,
    BuildAndExtractFn& build_and_extract,
    std::size_t max_clauses,
    std::size_t max_vars,
    std::size_t lower_limit_clauses,
    int depth)
{
    std::vector<std::vector<int>> extracted;
    if (build_and_extract(chunk, extracted)) {
        absorb_witnesses(extracted, unique_witnesses);
        return;
    }

    // At the recursion floor: further splitting is unlikely to help and would
    // produce chunks too small to yield useful witness clauses.
    if (chunk.size() <= lower_limit_clauses) {
        std::cout << "[destructive] depth=" << depth
                  << "  chunk of " << chunk.size()
                  << " clauses failed at lower limit (" << lower_limit_clauses
                  << "); giving up\n";
        return;
    }

    const std::size_t next_max_clauses = std::max<std::size_t>(max_clauses / 2, 1);
    const std::size_t next_max_vars    = std::max<std::size_t>(max_vars    / 2, 1);

    std::cout << "[destructive] depth=" << depth
              << "  chunk of " << chunk.size()
              << " clauses failed; resplitting with caps "
              << next_max_clauses << "/" << next_max_vars << "\n";

    auto sub_chunks = split_community(chunk, next_max_clauses, next_max_vars);
    for (auto& sub : sub_chunks) {
        destructive_extract_chunk(sub, unique_witnesses, build_and_extract,
                                  next_max_clauses, next_max_vars,
                                  lower_limit_clauses, depth + 1);
    }
}

} // namespace

// Splits community_clauses at the initial caps and dispatches each chunk to the
// recursive helper.
void destructive_extract(
    const std::vector<std::vector<int>>& community_clauses,
    std::set<std::vector<int>>& unique_witnesses,
    BuildAndExtractFn build_and_extract,
    std::size_t initial_max_clauses,
    std::size_t initial_max_vars,
    std::size_t lower_limit_clauses)
{
    auto chunks = split_community(community_clauses, initial_max_clauses, initial_max_vars);
    std::cout << "[destructive] community split into " << chunks.size()
              << " chunks (caps " << initial_max_clauses << "/" << initial_max_vars << ")\n\n";

    for (auto& chunk : chunks) {
        destructive_extract_chunk(chunk, unique_witnesses, build_and_extract,
                                  initial_max_clauses, initial_max_vars,
                                  lower_limit_clauses, 0);
    }
}

// Fine-splits the community, builds each piece, then merges pairs greedily by
// Jaccard overlap. Chunks that fail baseline build are dropped from the merge
// pool to avoid blocking the loop on unresolvable inputs.
void constructive_extract(
    const std::vector<std::vector<int>>& community_clauses,
    std::set<std::vector<int>>& unique_witnesses,
    BuildAndExtractFn build_and_extract,
    int         k_max_consecutive_failures,
    std::size_t initial_max_clauses,
    std::size_t initial_max_vars)
{
    auto initial = split_community(community_clauses, initial_max_clauses, initial_max_vars);
    std::cout << "[constructive] initial split: " << initial.size() << " chunks\n";

    std::unordered_map<int, Chunk> active;
    int next_id = 0;

    for (auto& clauses : initial) {
        std::vector<std::vector<int>> extracted;
        if (build_and_extract(clauses, extracted)) {
            absorb_witnesses(extracted, unique_witnesses);
            active.emplace(next_id++, make_chunk(std::move(clauses)));
        }
    }
    std::cout << "[constructive] " << active.size() << " chunks survived baseline build\n\n";

    // Greedy merge loop: repeatedly find and attempt the highest-Jaccard
    // untried pair. failed_pairs prevents retrying combinations that already
    // exceeded the node limit.
    std::set<std::pair<int, int>> failed_pairs;
    int consecutive_failures = 0;
    int successful_merges    = 0;
    int attempted_merges     = 0;

    while (consecutive_failures < k_max_consecutive_failures && active.size() >= 2)
    {
        // O(n^2) scan — n is the number of surviving chunks, typically small.
        int    best_a = -1, best_b = -1;
        double best_score = -1.0;

        std::vector<int> ids;
        ids.reserve(active.size());
        for (const auto& kv : active) ids.push_back(kv.first);

        for (std::size_t i = 0; i < ids.size(); ++i) {
            for (std::size_t j = i + 1; j < ids.size(); ++j) {
                const int a = ids[i], b = ids[j];
                if (failed_pairs.count(make_pair_key(a, b))) continue;
                const double s = jaccard(active[a].variables, active[b].variables);
                if (s > best_score) { best_score = s; best_a = a; best_b = b; }
            }
        }

        if (best_a == -1) break;

        ++attempted_merges;
        std::cout << "[constructive] attempt merge of chunks " << best_a << " + " << best_b
                  << " (jaccard = " << best_score << ")\n";

        std::vector<std::vector<int>> merged;
        merged.reserve(active[best_a].clauses.size() + active[best_b].clauses.size());
        merged.insert(merged.end(), active[best_a].clauses.begin(), active[best_a].clauses.end());
        merged.insert(merged.end(), active[best_b].clauses.begin(), active[best_b].clauses.end());

        std::vector<std::vector<int>> extracted;
        if (build_and_extract(merged, extracted)) {
            absorb_witnesses(extracted, unique_witnesses);
            active.erase(best_a);
            active.erase(best_b);
            active.emplace(next_id++, make_chunk(std::move(merged)));

            // Drop stale failed-pair entries that referenced the consumed chunks
            // so their ids are not matched against the new merged chunk.
            for (auto it = failed_pairs.begin(); it != failed_pairs.end(); ) {
                if (it->first == best_a || it->second == best_a
                 || it->first == best_b || it->second == best_b)
                    it = failed_pairs.erase(it);
                else
                    ++it;
            }
            consecutive_failures = 0;
            ++successful_merges;
        } else {
            failed_pairs.insert(make_pair_key(best_a, best_b));
            ++consecutive_failures;
        }
    }

    std::cout << "[constructive] merge loop done: "
              << successful_merges << "/" << attempted_merges
              << " merges succeeded; " << active.size() << " chunks remain\n\n";
}
