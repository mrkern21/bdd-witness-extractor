// up_check.cpp — measures how many witness clauses are "trivial", i.e.
// derivable from the original formula by unit propagation alone (RUP).
// A clause C is UP-implied by F iff adding the negated literals of C as
// assumptions to F and running unit propagation alone derives a conflict.
// Usage: up_check <original.cnf> <augmented.cnf>
// The augmented file's original-clause prefix is byte-identical to
// <original.cnf> (write_augmented_cnf copies it verbatim), so it is parsed
// only once, from the original file. The augmented file is scanned just up
// to its "c witness clauses:" marker and only the clauses after it — the
// witnesses — are parsed.

#include <cadical.hpp>

#include <cstdlib>
#include <fstream>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

// Parses only the witness clauses out of an augmented CNF: skips straight to
// the "c witness clauses:" marker written by cnf_writer.cpp, then parses
// everything after it. Never touches the original-clause prefix, which is
// already available from the original CNF file.
std::vector<std::vector<int>> parse_witness_clauses(const std::string& path) {
    std::ifstream in(path);
    if (!in) throw std::runtime_error("cannot open: " + path);

    std::string line;
    bool in_witness_section = false;
    std::vector<std::vector<int>> witnesses;
    std::vector<int> current;

    while (std::getline(in, line)) {
        if (!in_witness_section) {
            if (line.rfind("c witness clauses:", 0) == 0) in_witness_section = true;
            continue;
        }
        if (line.empty() || line[0] == 'c') continue;

        std::istringstream iss(line);
        int lit;
        while (iss >> lit) {
            if (lit == 0) {
                witnesses.push_back(current);
                current.clear();
            } else {
                current.push_back(lit);
            }
        }
    }

    if (!in_witness_section) {
        throw std::runtime_error("no 'c witness clauses:' marker found in " + path);
    }
    if (!current.empty()) witnesses.push_back(current);

    return witnesses;
}

int main(int argc, char** argv) {
    if (argc != 3) {
        std::cerr << "usage: " << argv[0] << " <original.cnf> <augmented.cnf>\n";
        return 1;
    }

    const std::string orig_path = argv[1];
    const std::string aug_path  = argv[2];

    std::vector<std::vector<int>> witnesses;
    try {
        witnesses = parse_witness_clauses(aug_path);
    } catch (const std::exception& e) {
        std::cerr << "parse error: " << e.what() << "\n";
        return 1;
    }

    CaDiCaL::Solver solver;
    solver.set("quiet", 1);
    solver.set("inprocessing", 0);
    solver.set("elim",         0);
    solver.set("subsume",      0);
    solver.set("vivify",       0);
    solver.set("probe",        0);
    solver.set("decompose",    0);


    int num_vars = 0;
    const char* err = solver.read_dimacs(orig_path.c_str(), num_vars);
    if (err) {
        std::cerr << "parse error: " << err << "\n";
        return 1;
    }

    const std::size_t num_orig      = static_cast<std::size_t>(solver.irredundant());
    const std::size_t num_witnesses = witnesses.size();

    std::cout << "original file    : " << orig_path << "\n";
    std::cout << "augmented file   : " << aug_path  << "\n";
    std::cout << "original clauses : " << num_orig  << "\n";
    std::cout << "witness clauses  : " << num_witnesses << "\n";

    std::size_t up_implied     = 0;
    std::size_t not_up_implied = 0;

    if (num_witnesses > 0) {
        // For each witness, assume the negation of every literal and cap
        // decisions at 0 so solve() can only run initial unit propagation from
        // those assumptions. Both 'assume' and 'limit' are reset by CaDiCaL after
        // every solve() call, so they must be re-applied on each iteration.
        // 20 (UNSAT) means UP alone derived a conflict, i.e. the witness is
        // UP-implied / trivial. Anything else (10 SAT, 0 limit hit) means UP
        // could not refute the negated witness, so it carries information
        // beyond UP.
        for (std::size_t i = 0; i < num_witnesses; ++i) {
            const auto& witness = witnesses[i];

            for (int lit : witness) solver.assume(-lit);
            solver.limit("decisions", 0);

            const int result = solver.solve();
            if (result == 20) ++up_implied;
            else              ++not_up_implied;
        }
    }

    const double useful_fraction = num_witnesses > 0
        ? static_cast<double>(not_up_implied) / num_witnesses
        : 0.0;

    std::cout << "\n=== UP-derivability summary ===\n";
    std::cout << "UP-implied (trivial)     : " << up_implied      << "\n";
    std::cout << "Not UP-implied (useful)  : " << not_up_implied  << "\n";
    std::cout << "Useful fraction          : " << (useful_fraction * 100.0) << "%\n";

    
    std::cout << "\nSUMMARY," << orig_path << "," << num_witnesses << ","
              << up_implied << "," << not_up_implied << "," << useful_fraction << "\n";

    return 0;
}