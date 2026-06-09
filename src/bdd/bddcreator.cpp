#include "bddcreator.hpp"
#include <cassert>

// Pre-builds var_to_layer so compute_next_state can check in O(1) whether a
// variable still has an unassigned layer below the current position.
BDDCreator::BDDCreator(std::vector<std::vector<int>> &clauses,
                       std::vector<int> &varorder, int MAX_NODES) : num_of_layers(static_cast<int>(varorder.size())),
                                                     clauses(clauses),
                                                     varorder(varorder),
                                                     layers(num_of_layers + 1),
                                                     state_table(num_of_layers + 1)
{
    for (int layer = 0; layer < num_of_layers; layer++)
    {
        int var = varorder[layer];
        var_to_layer[var] = layer;
    }
    this->num_of_nodes = 0;
    this->MAX_NODES = MAX_NODES;
}

// Deletes nodes only if ownership was not transferred to a BDD object.
// After create_bdd() succeeds, the BDD destructor is responsible for cleanup.
BDDCreator::~BDDCreator()
{
    if (!ownership_transferred)
    {
        for (size_t current_layer = 0; current_layer < layers.size(); current_layer++)
        {
            for (BDDNode *node : layers[current_layer])
            {
                delete node;
            }
        }
    }
}

// Runs the two construction phases in order and transfers node ownership to
// the returned BDD. After this call the BDDCreator must not delete the nodes.
BDD BDDCreator::create_bdd(void)
{
    BDDNode *root = top_down_construction();
    bottom_up_pass();
    this->ownership_transferred = true;
    return BDD(root, this->layers);
}

// Builds the BDD DAG layer by layer. Each node's state is the
// set of clauses not yet satisfied by the partial assignment leading to it.
// Nodes with identical normalized states are merged via state_table, keeping
// the graph compact. Throws std::runtime_error if MAX_NODES is exceeded.
BDDNode *BDDCreator::top_down_construction(void)
{
    int current_layer = 0;
    int current_var = varorder[current_layer];

    this->clauses = normalize_state(this->clauses);

    NodeStatus root_status = NodeStatus::DEFAULT;
    std::vector<int> empty_witness_clause;
    if (this->clauses.empty())
    {
        root_status = NodeStatus::SAT;
    }
    BDDNode *root = new BDDNode(current_var, current_layer, this->clauses, root_status, empty_witness_clause);

    layers[current_layer].push_back(root);
    state_table[current_layer][state_to_key(this->clauses)] = root;

    // now create remaining layers
    for (current_layer = 0; current_layer < num_of_layers; current_layer++)
    {
        int next_layer = current_layer + 1;
        int var_next_layer = current_layer != (num_of_layers - 1) ? varorder[next_layer] : -1;

        for (BDDNode *node : layers[current_layer])
        {
            NodeStatus current_status = node->get_status();

            if (current_status == NodeStatus::UNSAT || current_status == NodeStatus::SAT)
                continue;

            for (const bool value : this->domain)
            {
                if (this->num_of_nodes > this->MAX_NODES)
                {
                    throw std::runtime_error("MAX NODES REACHED in construction of BDD!");
                }

                NextState next_state = compute_next_state(current_var, value, node, next_layer);
                next_state.state = normalize_state(next_state.state);
                std::string normalized_state_string = state_to_key(next_state.state);

                BDDNode *equivalent_node = obtain_equivalent_node(next_state, next_layer);

                if (equivalent_node != nullptr)
                {
                    if (value)
                        node->set_high(equivalent_node);
                    else
                        node->set_low(equivalent_node);
                }
                else
                {
                    BDDNode *new_node = new BDDNode(var_next_layer, next_layer, next_state.state, next_state.status, next_state.witness_clause);
                    layers[next_layer].push_back(new_node);
                    state_table[next_layer][normalized_state_string] = new_node;

                    // increment node count
                    this->num_of_nodes++;

                    if (value)
                        node->set_high(new_node);
                    else
                        node->set_low(new_node);
                }
            }
        }
        current_var = var_next_layer;
    }

    return root;
}

// Propagates INFEASIBLE status upward from the deepest layer to the root.
// A node becomes INFEASIBLE when both its children are UNSAT or INFEASIBLE.
// Its witness clause is derived by resolution over the children's witness
// clauses, eliminating the branching variable. Nodes that become INFEASIBLE
// mark their children's has_infeasible_parent flag to prevent redundant
// witness collection in BDD::generate_witness_clauses().
void BDDCreator::bottom_up_pass(void)
{
    for (int current_layer = layers.size() - 1; current_layer >= 0; current_layer--)
    {
        for (BDDNode *node : layers[current_layer])
        {
            NodeStatus status = node->get_status();
            if (status != NodeStatus::UNSAT && status != NodeStatus::SAT)
            {
                BDDNode *low = node->get_low();
                BDDNode *high = node->get_high();

                if (low != nullptr && high != nullptr)
                {

                    // if low == high skip the computation as bad stuff could happen here
                    if (low == high)
                        continue;

                    NodeStatus low_status = low->get_status();
                    NodeStatus high_status = high->get_status();

                    if (low_status == NodeStatus::UNSAT || low_status == NodeStatus::INFEASIBLE)
                    {
                        if (high_status == NodeStatus::UNSAT || high_status == NodeStatus::INFEASIBLE)
                        {
                            node->set_status(NodeStatus::INFEASIBLE);
                            low->set_infeasible_parent(true);
                            high->set_infeasible_parent(true);

                            // compute witness_clause for node
                            std::vector<int> low_witness = low->get_witness_clause();
                            std::vector<int> high_witness = high->get_witness_clause();

                            int current_var = node->get_var();
                            if (!var_in_clause(current_var, low_witness))
                            {
                                node->set_witness_clause(low_witness);
                            }
                            else if (!var_in_clause(current_var, high_witness))
                            {
                                node->set_witness_clause(high_witness);
                            }
                            else
                            {
                                // Both witnesses contain current_var; resolve to eliminate it.
                                // If the resolvent is tautological, apply_resolution returns
                                // an empty vector and no witness clause is set for this node.
                                std::vector<int> new_witness = apply_resolution(low_witness, high_witness, current_var);
                                if (!new_witness.empty())
                                {
                                    node->set_witness_clause(new_witness);
                                }
                            }
                        }
                    }
                }
            }
        }
    }
}

// Applies the assignment var=value to the node's unsat_clauses and computes
// the resulting state for the child node. A clause is dropped from the state
// when it becomes satisfied; it causes UNSAT if it has no remaining literal
// at or below current_layer. The shortest unsatisfied clause is kept as the
// witness clause so resolution steps later produce compact learned clauses.
NextState BDDCreator::compute_next_state(int var, bool value, BDDNode *current_node, int current_layer)
{
    NextState result;
    result.status = NodeStatus::DEFAULT;

    for (const std::vector<int> &clause : current_node->get_unsat_clauses())
    {
        bool satisfied = false;
        bool has_unassigned_literal = false;

        for (const int literal : clause)
        {
            int current_var = std::abs(literal);

            if (variable_below_layer(current_var, current_layer))
            {
                has_unassigned_literal = true;
            }

            if ((literal > 0 && literal == var && value) ||
                (literal < 0 && literal == -var && !value))
            {
                satisfied = true;
                break;
            }
        }

        if (!satisfied)
        {
            if (!has_unassigned_literal)
            {
                result.status = NodeStatus::UNSAT;
                if (result.witness_clause.empty())
                {
                    result.witness_clause = clause;
                }
                else if (result.witness_clause.size() > clause.size())
                {
                    // choose shortest clause as witness clause for UNSAT node
                    result.witness_clause = clause;
                }
            }
            result.state.push_back(clause);
        }

        satisfied = false;
        has_unassigned_literal = false;
    }

    if (result.status == NodeStatus::UNSAT)
    {
        return result;
    }

    if (result.state.empty())
    {
        result.status = NodeStatus::SAT;
        return result;
    }

    return result;
}

// Looks up the normalized state in the per-layer hash table. Returns the
// existing node only if its status also matches; a same-state node with a
// different status (e.g. DEFAULT vs UNSAT) is not considered equivalent
// because its subtree and witness clause would differ.
BDDNode *BDDCreator::obtain_equivalent_node(NextState &state, int next_layer)
{
    std::vector<std::vector<int>> &current_state = state.state;

    std::string key = state_to_key(current_state);

    auto it = state_table[next_layer].find(key);

    if (it != state_table[next_layer].end())
    {

        BDDNode *node = it->second;

        if (state.status == node->get_status())
        {
            return node;
        }
    }

    return nullptr;
}

// Sorts literals within each clause and then sorts the clauses themselves so
// that two states representing the same unsatisfied set always produce an
// identical key, regardless of the order clauses were propagated in.
std::vector<std::vector<int>> BDDCreator::normalize_state(const std::vector<std::vector<int>> &state)
{
    std::vector<std::vector<int>> normalized = state;

    for (std::vector<int> &clause : normalized)
    {
        std::sort(clause.begin(), clause.end());
    }

    std::sort(normalized.begin(), normalized.end());

    return normalized;
}

// Serializes a normalized state to a string for use as a hash map key.
// Must be called on a normalized state (via normalize_state) to guarantee
// that structurally equal states produce the same key.
std::string BDDCreator::state_to_key(const std::vector<std::vector<int>> &state)
{
    std::string key;

    for (const std::vector<int> &clause : state)
    {
        key += "[";
        for (const int lit : clause)
        {
            key += std::to_string(lit);
            key += " ";
        }
        key.pop_back();
        key += "]";
    }

    return key;
}

// Returns true if var appears at or below the given layer, meaning it has
// not been assigned yet at this point in top-down construction. Used by
// compute_next_state to distinguish falsified clauses (no unassigned literals
// left) from still-open ones.
bool BDDCreator::variable_below_layer(int var, int layer)
{
    return var_to_layer[var] >= layer;
}

// Checks by absolute value whether var (or its negation) appears in clause.
// Used by bottom_up_pass to decide whether resolution is needed or whether
// one child's witness clause already lacks the branching variable.
bool BDDCreator::var_in_clause(int var, const std::vector<int> &clause)
{
    for (int lit : clause)
    {
        int current_var = std::abs(lit);
        if (current_var == var)
        {
            return true;
        }
    }
    return false;
}

// Applies the resolution rule: merges low_witness and high_witness by
// eliminating all occurrences of var (positive and negative), deduplicates,
// and returns the remaining literals sorted. std::set provides both
// deduplication and sorted order in one step, so no separate sort is needed.
// Returns an empty vector if the resolvent is tautological (i.e. some other
// variable appears with both polarities in the merged result), signalling to
// the caller that no useful witness clause can be derived from this resolution.
std::vector<int> BDDCreator::apply_resolution(const std::vector<int> &low_witness, const std::vector<int> &high_witness, int var)
{
    assert(var_in_clause(var, low_witness) && var_in_clause(var, high_witness));

    std::set<int> tmp;

    for (int lit : low_witness)
    {
        if (std::abs(lit) != var)
        {
            tmp.insert(lit);
        }
    }

    for (int lit : high_witness)
    {
        if (std::abs(lit) != var)
        {
            tmp.insert(lit);
        }
    }

    // Tautology check: if any variable appears with both polarities, the
    // resolvent is always true and useless as a learned clause.
    for (int lit : tmp)
    {
        if (tmp.count(-lit))
        {
            return {};
        }
    }

    return std::vector<int>(tmp.begin(), tmp.end());
}