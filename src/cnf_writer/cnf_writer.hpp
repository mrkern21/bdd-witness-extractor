// cnf_writer.hpp — writes an augmented DIMACS CNF file.
// Copies the original instance line by line, rewrites the "p cnf n m" header
// to reflect the added witness clauses (only m is updated; n is unchanged since
// witness clauses only reference variables already in the formula), then appends
// the witness clauses in standard DIMACS format.
#ifndef __CNF_WRITER_HPP
#define __CNF_WRITER_HPP

#include <string>
#include <vector>

// Writes the original CNF at original_path with witnesses appended to output_path.
// witness literals follow DIMACS convention; the trailing 0 per clause is added
// on write, so inner vectors must not include it.
// Throws std::runtime_error if either file cannot be opened or the original
// has no valid "p cnf <n> <m>" header line.
void write_augmented_cnf(
    const std::string& original_path,
    const std::vector<std::vector<int>>& witnesses,
    const std::string& output_path);

#endif