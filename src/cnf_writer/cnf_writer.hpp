#ifndef __CNF_WRITER_HPP
#define __CNF_WRITER_HPP

// =============================================================================
// cnf_writer.hpp
// -----------------------------------------------------------------------------
// Writes an augmented DIMACS CNF file: original instance + appended witness
// clauses. Used to produce the artifact that the comparison script (CaDiCaL
// stats baseline vs augmented) consumes.
//
// The variable count `n` in the header is preserved unchanged — witness
// clauses only reference variables already in the formula. Only the clause
// count `m` is rewritten to `m + #witnesses`.
// =============================================================================

#include <string>
#include <vector>

// Write an augmented CNF.
//
//   original_path : path to the original DIMACS file (read line by line)
//   witnesses     : witness clauses to append (DIMACS literal convention,
//                   no trailing 0 expected in the inner vectors — added on write)
//   output_path   : where to write the result
//
// Throws std::runtime_error if either file can't be opened or the original
// has no valid "p cnf <n> <m>" header line.
void write_augmented_cnf(
    const std::string& original_path,
    const std::vector<std::vector<int>>& witnesses,
    const std::string& output_path);

#endif