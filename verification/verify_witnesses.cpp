// verify_witnesses.cpp — correctness checker for extracted witness clauses.
//
// Usage: verify_witnesses <original.cnf> <augmented.cnf> <sat|unsat>
//
// The augmented CNF produced by bdd_witness_extractor contains the full
// original formula followed by the witness clauses:
//
//   p cnf <n> <m+k>
//   <original m clauses>
//   c k witness clauses appended
//   <k witness clauses>
//
// The original formula is read from <original.cnf> directly; the witness
// clauses are taken as everything in <augmented.cnf> after the first m
// clauses, where m is the number of clauses in <original.cnf>.
//
// SAT mode:
//   Runs CaDiCaL on the augmented CNF.  If it finds a model, that model is
//   checked against every clause of the original instance to confirm it is
//   a valid model of the original instance.
//
// UNSAT mode:
//   For each witness clause w = (l1 ∨ … ∨ lk), checks that
//   original ∧ ¬l1 ∧ … ∧ ¬lk  is UNSAT, which is equivalent to
//   original ⊨ w  (w is a logical consequence of the original formula).
//
// Exit codes: 0 = all checks passed, 1 = at least one check failed,
//             2 = usage / parse error.

#include <cadical.hpp>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

struct ParsedCNF {
    int num_vars = 0;
    std::vector<std::vector<int>> clauses;
};

// Parses a plain DIMACS CNF file into its variable count and clause list.
static ParsedCNF parse_cnf(const std::string &path)
{
    std::ifstream in(path);
    if (!in) {
        std::cerr << "error: cannot open " << path << "\n";
        std::exit(2);
    }

    ParsedCNF result;
    std::string line;
    std::vector<int> cur;

    while (std::getline(in, line)) {
        if (line.empty()) continue;
        if (line[0] == 'c') continue;
        if (line[0] == 'p') {
            std::istringstream ss(line);
            std::string p, cnf;
            ss >> p >> cnf >> result.num_vars;
            continue;
        }
        std::istringstream ss(line);
        int lit;
        while (ss >> lit) {
            if (lit == 0) {
                if (!cur.empty()) { result.clauses.push_back(cur); cur.clear(); }
            } else {
                cur.push_back(lit);
            }
        }
    }
    if (!cur.empty()) result.clauses.push_back(cur);

    return result;
}

// Literal l is satisfied by a CaDiCaL model iff val(|l|) has the same sign.
static bool lit_sat(CaDiCaL::Solver &s, int l)
{
    return s.val(std::abs(l)) == l;
}

// SAT path: solve the augmented CNF, then verify the model clause-by-clause
// against the original formula.
static bool verify_sat(const std::vector<std::vector<int>> &original_clauses,
                        const std::string &aug_path)
{
    CaDiCaL::Solver solver;
    solver.set("verbose", 0);

    int nvars = 0;
    const char *err = solver.read_dimacs(aug_path.c_str(), nvars);
    if (err) {
        std::cerr << "error reading augmented CNF: " << err << "\n";
        return false;
    }

    int res = solver.solve();
    if (res != 10) {
        std::cout << "[FAIL] augmented formula is UNSAT — no model to check\n";
        return false;
    }

    std::cout << "augmented formula is SAT; verifying model against "
              << original_clauses.size() << " original clauses...\n";

    int failures = 0;
    for (std::size_t i = 0; i < original_clauses.size(); ++i) {
        bool satisfied = false;
        for (int l : original_clauses[i]) {
            if (lit_sat(solver, l)) { satisfied = true; break; }
        }
        if (!satisfied) {
            std::cout << "  [FAIL] clause " << (i + 1) << ":";
            for (int l : original_clauses[i]) std::cout << " " << l;
            std::cout << " 0\n";
            ++failures;
        }
    }

    if (failures == 0) {
        std::cout << "[ok] model satisfies all " << original_clauses.size()
                  << " original clauses\n";
        return true;
    }
    std::cout << "[FAIL] " << failures << " original clause(s) not satisfied\n";
    return false;
}

// UNSAT path: for each witness clause w, verify original ⊨ w by checking
// that original ∧ ¬w is UNSAT.
static bool verify_unsat(const std::string &orig_path,
                          const std::vector<std::vector<int>> &witness_clauses)
{
    if (witness_clauses.empty()) {
        std::cout << "no witness clauses found — nothing to verify\n";
        return true;
    }

    std::cout << "checking entailment of " << witness_clauses.size()
              << " witness clauses (original |= w  iff  original ∧ ¬w is UNSAT)...\n";

    int pass = 0, fail = 0;
    for (std::size_t i = 0; i < witness_clauses.size(); ++i) {
        const auto &w = witness_clauses[i];

        CaDiCaL::Solver solver;
        solver.set("verbose", 0);
        solver.set("quiet", 1);  // suppress read_dimacs's per-call parse banner

        int nvars = 0;
        const char *err = solver.read_dimacs(orig_path.c_str(), nvars);
        if (err) {
            std::cerr << "error reading original CNF: " << err << "\n";
            return false;
        }

        // ¬w = ¬l1 ∧ … ∧ ¬lk — add each negated literal as a unit clause.
        for (int l : w) {
            solver.add(-l);
            solver.add(0);
        }

        int res = solver.solve();
        if (res == 20) {
            std::cout << "  [ok]   witness " << (i + 1) << ":";
            for (int l : w) std::cout << " " << l;
            std::cout << " 0\n";
            ++pass;
        } else {
            std::cout << "  [FAIL] witness " << (i + 1) << ":";
            for (int l : w) std::cout << " " << l;
            std::cout << " 0  (original ∧ ¬w is SAT — not entailed)\n";
            ++fail;
        }
    }

    std::cout << "\n" << pass << "/" << witness_clauses.size()
              << " witness clauses entailed by the original formula\n";
    return fail == 0;
}

int main(int argc, char *argv[])
{
    if (argc != 4) {
        std::cerr << "usage: " << argv[0]
                  << " <original.cnf> <augmented.cnf> <sat|unsat>\n";
        return 2;
    }

    const std::string orig_path = argv[1];
    const std::string aug_path  = argv[2];
    const std::string mode      = argv[3];

    if (mode != "sat" && mode != "unsat") {
        std::cerr << "error: mode must be 'sat' or 'unsat'\n";
        return 2;
    }

    ParsedCNF orig = parse_cnf(orig_path);
    ParsedCNF aug  = parse_cnf(aug_path);

    if (aug.clauses.size() < orig.clauses.size()) {
        std::cerr << "error: augmented CNF has fewer clauses than original CNF\n";
        return 2;
    }

    std::vector<std::vector<int>> witness_clauses(
        aug.clauses.begin() + static_cast<long>(orig.clauses.size()),
        aug.clauses.end());

    std::cout << "original clauses : " << orig.clauses.size() << "\n";
    std::cout << "witness clauses  : " << witness_clauses.size()  << "\n\n";

    bool ok = (mode == "sat")
        ? verify_sat(orig.clauses, aug_path)
        : verify_unsat(orig_path, witness_clauses);

    return ok ? 0 : 1;
}
