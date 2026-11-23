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
// The verifier first checks that <augmented.cnf> really starts with the
// clauses of <original.cnf>, so the two files belong together. It then solves
// both files with CaDiCaL and checks that both give the expected result
// (sat or unsat), i.e. that the witness clauses did not change the solver
// result.
//
// Note: this only checks that the solver result is unchanged. It does NOT
// explicitly check entailment (original ⊨ w for each witness clause w). In
// the SAT case, a satisfiable augmented formula shows that some model of the
// original satisfies the witness clauses, not that every model does.
//
// Exit codes: 0 = all checks passed, 1 = at least one check failed,
//             2 = usage / parse error.

#include <cadical.hpp>
#include <algorithm>
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

// Reads a DIMACS file into a fresh solver and solves it. Returns CaDiCaL's
// result code (10 = SAT, 20 = UNSAT, 0 = unknown), or -1 on a read error.
static int solve_file(const std::string &path)
{
    CaDiCaL::Solver solver;
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
    const int expected = (mode == "sat") ? 10 : 20;

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

    std::cout << "original clauses : " << orig.size() << "\n";
    std::cout << "witness clauses  : " << aug.size() - orig.size() << "\n\n";

    const int orig_res = solve_file(orig_path);
    if (orig_res < 0) return 2;
    std::cout << "original  : " << result_to_string(orig_res) << "\n";

    const int aug_res = solve_file(aug_path);
    if (aug_res < 0) return 2;
    std::cout << "augmented : " << result_to_string(aug_res) << "\n\n";

    if (orig_res != expected || aug_res != expected) {
        std::cout << "[FAIL] expected both to be " << result_to_string(expected)
                  << "\n";
        return 1;
    }

    std::cout << "[ok] original and augmented formula are both "
              << result_to_string(expected) << "\n";
    return 0;
}
