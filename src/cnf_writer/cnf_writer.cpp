#include "cnf_writer.hpp"

#include <fstream>
#include <sstream>
#include <stdexcept>
#include <string>

void write_augmented_cnf(
    const std::string& original_path,
    const std::vector<std::vector<int>>& witnesses,
    const std::string& output_path)
{
    // ------- open both files up front so we fail before doing any work -------
    std::ifstream in(original_path);
    if (!in) {
        throw std::runtime_error("cannot open input CNF: " + original_path);
    }
    std::ofstream out(output_path);
    if (!out) {
        throw std::runtime_error("cannot open output CNF: " + output_path);
    }

    // ------- copy lines through, rewriting the p-cnf header when we see it ---
    // A DIMACS file may have any number of comment lines ('c ...') before the
    // header. The header is the first line starting with "p cnf". Some files
    // have extra whitespace, some have trailing content; we use istringstream
    // so this all works without surprises.
    bool   header_rewritten = false;
    std::string line;

    while (std::getline(in, line)) {
        // Detect the header by its prefix. Anything else we copy verbatim.
        if (!header_rewritten && line.rfind("p cnf", 0) == 0) {
            std::istringstream iss(line);
            std::string p_token, cnf_token;
            int n = 0, m = 0;
            iss >> p_token >> cnf_token >> n >> m;
            if (!iss || p_token != "p" || cnf_token != "cnf" || n < 0 || m < 0) {
                throw std::runtime_error("malformed CNF header: " + line);
            }

            const std::size_t new_m = static_cast<std::size_t>(m) + witnesses.size();
            out << "p cnf " << n << " " << new_m << "\n";
            header_rewritten = true;
            continue;
        }
        out << line << "\n";
    }

    if (!header_rewritten) {
        throw std::runtime_error(
            "no 'p cnf' header found in input: " + original_path);
    }

    // ------- append witnesses in DIMACS clause format -----------------------
    // Each clause: space-separated literals followed by a terminating 0.
    for (const auto& clause : witnesses) {
        for (int lit : clause) {
            out << lit << ' ';
        }
        out << "0\n";
    }

    if (!out) {
        throw std::runtime_error("error while writing: " + output_path);
    }
}
