// verify_witnesses.cpp — correctness checker for extracted witness clauses.
//
// Usage: verify_witnesses <original.cnf> <augmented.cnf> <sat|unsat>
//
// The augmented CNF produced by bdd_witness_extractor contains the full
// original formula followed by the witness clauses:
//
//   c k witness clauses appended
//   p cnf <n> <m+k>
//   <original m clauses>
//   c witness clauses;
//   <k witness clauses>
//
// The verifier first checks that <augmented.cnf> really starts with the
// clauses of <original.cnf>, so the two files belong together. Everything
// after the original clauses is taken as a witness clause.
//
// SAT mode:
//   Solves the original CNF with CaDiCaL and checks that the model it finds
//   also satisfies every witness clause. That model is then a model of the
//   augmented formula too, so the solver result is unchanged (both SAT).
//   A violated witness clause is definitely not entailed by the original,
//   since every model of the original would have to satisfy an entailed one.
//
// UNSAT mode:
//   Solves both the original and the augmented CNF with CaDiCaL and checks
//   that both are UNSAT, i.e. that the witness clauses did not change the
//   solver result.
//
// Note: these are sanity checks that the solver result is unchanged. They do
// NOT explicitly check entailment (original ⊨ w for each witness clause w).
// In SAT mode only one model of the original is tested, not every model.
//
// Exit codes: 0 = all checks passed, 1 = at least one check failed,
//             2 = usage / parse error.

#include <cadical.hpp>
#include <algorithm>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

using Clauses = std::vector<std::vector<int>>;

// Parses the clauses of a plain DIMACS CNF file. Returns false if the file
// cannot be opened.
static bool parse_cnf(const std::string &path, Clauses &clauses)
{
    std::ifstream in(path);
    if (!in) {
        std::cerr << "error: cannot open " << path << "\n";
        return false;
    }

    std::string line;
    std::vector<int> cur;

    while (std::getline(in, line)) {
        if (line.empty() || line[0] == 'c' || line[0] == 'p') continue;
        std::istringstream ss(line);
        int lit;
        while (ss >> lit) {
            if (lit == 0) {
                if (!cur.empty()) { clauses.push_back(cur); cur.clear(); }
            } else {
                cur.push_back(lit);
            }
        }
    }
    if (!cur.empty()) clauses.push_back(cur);

    return true;
}

// Maps a CaDiCaL result code to a readable string.
static const char *result_to_string(int res)
{
    if (res == 10) return "SAT";
    if (res == 20) return "UNSAT";
    return "UNKNOWN";
}

// Reads a DIMACS file into the given solver and solves it. Returns CaDiCaL's
// result code (10 = SAT, 20 = UNSAT, 0 = unknown), or -1 on a read error.
static int solve_file(CaDiCaL::Solver &solver, const std::string &path)
{
    solver.set("verbose", 0);
    solver.set("quiet", 1);  // suppress read_dimacs's parse banner

    int nvars = 0;
    const char *err = solver.read_dimacs(path.c_str(), nvars);
    if (err) {
        std::cerr << "error reading " << path << ": " << err << "\n";
        return -1;
    }
    return solver.solve();
}

// A literal is true in the model iff its variable's value has the same sign.
// (val() of a positive variable index is positive iff the variable is true.)
static bool lit_true(CaDiCaL::Solver &solver, int lit)
{
    return (solver.val(std::abs(lit)) > 0) == (lit > 0);
}

// SAT path: solve the original CNF and check that its model satisfies every
// witness clause. Returns an exit code.
static int verify_sat(const std::string &orig_path, const Clauses &witnesses)
{
    CaDiCaL::Solver solver;
    const int res = solve_file(solver, orig_path);
    if (res < 0) return 2;
    std::cout << "original  : " << result_to_string(res) << "\n\n";

    if (res != 10) {
        std::cout << "[FAIL] expected original to be SAT\n";
        return 1;
    }

    int failures = 0;
    for (std::size_t i = 0; i < witnesses.size(); ++i) {
        const auto &w = witnesses[i];
        const bool satisfied = std::any_of(w.begin(), w.end(),
            [&](int lit) { return lit_true(solver, lit); });
        if (!satisfied) {
            std::cout << "  [FAIL] witness " << (i + 1) << ":";
            for (int lit : w) std::cout << " " << lit;
            std::cout << " 0  (violated by the model of the original — not entailed)\n";
            ++failures;
        }
    }

    if (failures > 0) {
        std::cout << "[FAIL] " << failures << "/" << witnesses.size()
                  << " witness clause(s) violated by the model of the original\n";
        return 1;
    }

    std::cout << "[ok] model of the original satisfies all " << witnesses.size()
              << " witness clauses, so original and augmented formula are both SAT\n";
    return 0;
}

// UNSAT path: solve both the original and the augmented CNF and check that
// both are UNSAT. Returns an exit code.
static int verify_unsat(const std::string &orig_path, const std::string &aug_path)
{
    CaDiCaL::Solver orig_solver;
    const int orig_res = solve_file(orig_solver, orig_path);
    if (orig_res < 0) return 2;
    std::cout << "original  : " << result_to_string(orig_res) << "\n";

    CaDiCaL::Solver aug_solver;
    const int aug_res = solve_file(aug_solver, aug_path);
    if (aug_res < 0) return 2;
    std::cout << "augmented : " << result_to_string(aug_res) << "\n\n";

    if (orig_res != 20 || aug_res != 20) {
        std::cout << "[FAIL] expected both to be UNSAT\n";
        return 1;
    }

    std::cout << "[ok] original and augmented formula are both UNSAT\n";
    return 0;
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

    // The augmented file must start with exactly the original clauses;
    // everything after them is a witness clause.
    Clauses orig, aug;
    if (!parse_cnf(orig_path, orig) || !parse_cnf(aug_path, aug)) return 2;

    if (aug.size() < orig.size()
        || !std::equal(orig.begin(), orig.end(), aug.begin())) {
        std::cerr << "error: augmented CNF does not start with the clauses "
                     "of the original CNF\n";
        return 2;
    }

    const Clauses witnesses(aug.begin() + static_cast<long>(orig.size()), aug.end());

    std::cout << "original clauses : " << orig.size() << "\n";
    std::cout << "witness clauses  : " << witnesses.size() << "\n\n";

    return (mode == "sat") ? verify_sat(orig_path, witnesses)
                           : verify_unsat(orig_path, aug_path);
}
