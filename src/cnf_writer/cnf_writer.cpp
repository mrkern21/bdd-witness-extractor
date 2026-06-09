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
    // Open both files before doing any work so we fail fast without writing
    // a partial output file.
    std::ifstream in(original_path);
    if (!in) {
        throw std::runtime_error("cannot open input CNF: " + original_path);
    }
    std::ofstream out(output_path);
    if (!out) {
        throw std::runtime_error("cannot open output CNF: " + output_path);
    }

    // Copy lines verbatim, rewriting only the "p cnf" header.
    // A DIMACS file may have any number of 'c' comment lines before the header.
    // istringstream handles files with extra whitespace or trailing content.
    bool   header_rewritten = false;
    std::string line;

    while (std::getline(in, line)) {
        if (!header_rewritten && line.rfind("p cnf", 0) == 0) {
            std::istringstream iss(line);
            std::string p_token, cnf_token;
            int n = 0, m = 0;
            iss >> p_token >> cnf_token >> n >> m;
            if (!iss || p_token != "p" || cnf_token != "cnf" || n < 0 || m < 0) {
                throw std::runtime_error("malformed CNF header: " + line);
            }

            out << "c " << witnesses.size() << " witness clauses appended\n";
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

    // Append witness clauses; add the DIMACS terminating 0 per clause here
    // since inner vectors do not carry it.
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