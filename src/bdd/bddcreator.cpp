#include "bddcreator.hpp"

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

BDD BDDCreator::create_bdd(void)
{
    BDDNode *root = top_down_construction();
    bottom_up_pass();
    this->ownership_transferred = true;
    return BDD(root, this->layers);
}

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
                                // compute new witness clause by applying the resolution rule
                                // assert that one clause contains x_i and the other ~x_i
                                std::vector<int> new_witness = apply_resolution(low_witness, high_witness, current_var);
                                node->set_witness_clause(new_witness);
                            }
                        }
                    }
                }
            }
        }
    }
}

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

bool BDDCreator::variable_below_layer(int var, int layer)
{
    return var_to_layer[var] >= layer;
}

bool BDDCreator::var_in_clause(int var, std::vector<int> &clause)
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

std::vector<int> BDDCreator::apply_resolution(const std::vector<int> &low_witness, const std::vector<int> &high_witness, int var)
{
    std::vector<int> new_witness;
    std::unordered_set<int> tmp;

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

    new_witness.reserve(tmp.size());
    for (auto it = tmp.begin(); it != tmp.end();)
    {
        new_witness.push_back(std::move(tmp.extract(it++).value()));
    }

    std::sort(new_witness.begin(), new_witness.end());

    return new_witness;
}