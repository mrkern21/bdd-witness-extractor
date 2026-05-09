#include "kondylidou_var_order.hpp"

Kondy_VarOrder::Kondy_VarOrder(std::vector<std::vector<int>>& clauses) : clauses(clauses)
{
}

std::vector<int> Kondy_VarOrder::create_var_order()
{
        // Per-variable accumulators.
    std::unordered_map<int, int>      clause_count;  // # clauses containing v
    std::unordered_map<int, long long> arity_sum;    // sum of those clauses' sizes
 
    // Reusable per-clause de-duplication set: a variable appearing twice in
    // the same clause (e.g. via a tautology like x ∨ ¬x) should still count
    // as one occurrence within that clause.
    std::unordered_set<int> seen;
 
    for (const auto& clause : clauses) {
        const int arity = static_cast<int>(clause.size());
        if (arity == 0) continue;  // empty clauses have no variables
 
        seen.clear();
        for (int lit : clause) {
            const int v = std::abs(lit);
            if (v == 0) continue;  // defensive against stray DIMACS terminators
 
            if (seen.insert(v).second) {
                ++clause_count[v];
                arity_sum[v] += arity;
            }
        }
    }
 
    // Compute the score for each variable once and store alongside the id,
    // so the comparator below doesn't recompute scores at every comparison.
    struct VarScore { int var; double score; };
    std::vector<VarScore> scored;
    scored.reserve(clause_count.size());
 
    for (const auto& kv : clause_count) {
        const int    v         = kv.first;
        const int    count     = kv.second;
        const double avg_arity = static_cast<double>(arity_sum[v]) / count;
        const double score     = count / avg_arity;
        scored.push_back({v, score});
    }
 
    // Sort: highest score first; ties broken by smaller variable id.
    std::sort(scored.begin(), scored.end(),
        [](const VarScore& a, const VarScore& b) {
            if (a.score != b.score) return a.score > b.score;
            return a.var < b.var;
        });
 
    // Project down to just the variable ids in BDD-top-to-bottom order.
    std::vector<int> order;
    order.reserve(scored.size());
    for (const auto& s : scored) order.push_back(s.var);
    return order;
}