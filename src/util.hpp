#ifndef __UTIL_HPP
#define __UTIL_HPP

#include <iostream>
#include <unordered_map>
#include <vector>


class BDD;

class BDDCreator;

class Kondy_VarOrder;

enum class NodeStatus {
    DEFAULT,
    SAT,
    UNSAT,
    INFEASIBLE
};

enum class Mode {
    Constructive,
    Destructive,
};

struct NextState {
    std::vector<std::vector<int>> state;
    std::vector<int> witness_clause;
    NodeStatus status;
};

struct bdd_stats {
    int num_of_nodes;
    int num_of_sat_nodes;
    int num_of_unsat_nodes;
    int num_of_infeasible_nodes;
    int num_of_default_nodes;
};

struct ProgramArgs {
    std::string input_path = "instance.cnf";   // <-- change to your preferred default
    int         max_nodes  = 10'000'000;        // <-- change to your preferred default
    Mode        mode       = Mode::Destructive; // default per request
};

std::string status_to_string(NodeStatus status);

std::string clause_to_string(std::vector<int>& clause);

void print_witness_clauses(std::vector<std::vector<int>>& clauses);

void print_bdd_stats(BDD& bdd);

void print_sat_result(int result);

std::vector<std::vector<std::vector<int>>> split_community(
    const std::vector<std::vector<int>>& community_clauses,
    std::size_t max_clauses = 300,
    std::size_t max_vars    = 95);

bool try_build_and_extract(
    std::vector<std::vector<int>> &chunk,
    std::vector<std::vector<int>> &witness_clauses,
    int MAX_NODES);

ProgramArgs parse_args(int argc, char** argv);

#endif