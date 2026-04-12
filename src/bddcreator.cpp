#include "bddcreator.hpp"

BDDCreator::BDDCreator(std::vector<std::vector<int>>& clauses,
                       std::queue<int>& varorder) :
                       num_of_layers(static_cast<int>(varorder.size())),
                       clauses(clauses),
                       varorder(varorder),
                       layers(num_of_layers + 1),
                       state_table(num_of_layers + 1)
{
}

BDD BDDCreator::create_bdd(void)
{
    int current_layer = 0;
    int current_var = varorder.front();
    varorder.pop();

    std::vector<BDDNode*> unsat_nodes;
    this->clauses = normalize_state(this->clauses);
    BDDNode* root = new BDDNode(current_var, current_layer, this->clauses);
    layers[current_layer].push_back(root);
    state_table[current_layer][state_to_key(this->clauses)] = root;

    // now create remaining layers
    for (current_layer = 0; current_layer < num_of_layers; current_layer++)
    {
        int var_next_layer = varorder.front();
        int next_layer = current_layer + 1;

        for (BDDNode* node : layers[current_layer])
        {
            if (node->is_unsat()) continue;
            
            for (bool value : this->domain)
            {
                std::vector<std::vector<int>> new_state = compute_next_state(current_var, value, node);
                new_state = normalize_state(new_state); 
                std::string normalized_state_string = state_to_key(new_state);

                //check if node is SAT, UNSAT, INFEASIBLE

                BDDNode* equivalent_node = obtain_equivalent_node(new_state, next_layer);

                if (equivalent_node != nullptr)
                {
                    if (value) node->set_high(equivalent_node);
                    else node->set_low(equivalent_node);
                }
                else
                {
                    BDDNode* new_node = new BDDNode(var_next_layer, next_layer, new_state);
                    layers[next_layer].push_back(new_node);
                    state_table[next_layer][normalized_state_string] = new_node;

                    if (value) node->set_high(new_node);
                    else node->set_low(new_node);
                }
            }
        }

        current_var = var_next_layer;
        varorder.pop(); 
    }


    return BDD(root, unsat_nodes);
}

std::vector<std::vector<int>> BDDCreator::compute_next_state(int var, bool value, BDDNode* current_node)
{
    std::vector<std::vector<int>> new_state;
    for (const std::vector<int>& clause : current_node->get_unsat_clauses())
    {
        bool satisfied = false;

        for (int literal : clause)
        {
            if ((literal > 0 && literal == var && value) ||
                (literal < 0 && literal == -var && !value))
            {
                satisfied = true;
                break;
            }
        }
        if (!satisfied)
        {
            new_state.push_back(clause);
        }
        satisfied = false;
    }

    return new_state;
}

BDDNode* BDDCreator::obtain_equivalent_node(const std::vector<std::vector<int>> &current_state, int next_layer)
{
    std::string key = state_to_key(current_state);

    auto it = state_table[next_layer].find(key);

    if(it != state_table[next_layer].end()){
        return it->second;
    }

    return nullptr;
}

std::vector<std::vector<int>> BDDCreator::normalize_state(const std::vector<std::vector<int>>& state)
{
    std::vector<std::vector<int>> normalized = state;

    for(std::vector<int>& clause : normalized){
        std::sort(clause.begin(), clause.end());
    }

    std::sort(normalized.begin(), normalized.end());

    return normalized;
}

std::string BDDCreator::state_to_key(const std::vector<std::vector<int>>& state)
{
    std::string key;

    for(const std::vector<int>& clause : state) {
        key += "[";
        for(int lit : clause) {
            key += std::to_string(lit);
            key += " ";
        }
        key += "]";
    }

    return key;
}