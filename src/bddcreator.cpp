#include "bddcreator.hpp"

BDDCreator::BDDCreator(std::vector<std::vector<int>>& clauses,
                       std::vector<int>& varorder) :
                       num_of_layers(static_cast<int>(varorder.size())),
                       clauses(clauses),
                       varorder(varorder),
                       layers(num_of_layers + 1),
                       state_table(num_of_layers + 1)
{
    for(int layer = 0; layer < num_of_layers; layer++){
        int var = varorder[layer];
        var_to_layer[var] = layer;
    }
}

BDD BDDCreator::create_bdd(void)
{
    int current_layer = 0;
    int current_var = varorder[current_layer];

    std::vector<BDDNode*> unsat_nodes;
    this->clauses = normalize_state(this->clauses);

    NodeStatus root_status = NodeStatus::DEFAULT;
    if(this->clauses.empty()){
        root_status = NodeStatus::SAT;
    }
    BDDNode* root = new BDDNode(current_var, current_layer, this->clauses, root_status);

    std::unordered_map<int, int> root_assignment;
    for(int layer = 0; layer < num_of_layers; layer++){
        root_assignment[varorder[layer]] = 0;
    }

    layers[current_layer].push_back({root, root_assignment});
    state_table[current_layer][state_to_key(this->clauses)] = root;

    // now create remaining layers
    for (current_layer = 0; current_layer < num_of_layers; current_layer++)
    {
        int next_layer = current_layer + 1;
        int var_next_layer = current_layer != (num_of_layers - 1) ? varorder[next_layer] : -1;

        for (const Working_Node& assignment : layers[current_layer])
        {
            BDDNode* node = assignment.node;
            NodeStatus current_status = node->get_status();

            if (current_status == NodeStatus::UNSAT || current_status == NodeStatus::SAT) continue;
            
            for (const bool value : this->domain)
            {
                std::unordered_map<int, int> new_assignment = assignment.assignment;
                new_assignment[current_var] = value ? 1 : -1;

                NextState next_state = compute_next_state(current_var, value, node, new_assignment, next_layer);
                next_state.state = normalize_state(next_state.state); 
                std::string normalized_state_string = state_to_key(next_state.state);

                BDDNode* equivalent_node = obtain_equivalent_node(next_state, next_layer);

                if (equivalent_node != nullptr)
                {
                    if (value) node->set_high(equivalent_node);
                    else node->set_low(equivalent_node);
                }
                else
                {
                    BDDNode* new_node = new BDDNode(var_next_layer, next_layer, next_state.state, next_state.status);
                    layers[next_layer].push_back({new_node, std::move(new_assignment)});
                    state_table[next_layer][normalized_state_string] = new_node;

                    if (value) node->set_high(new_node);
                    else node->set_low(new_node);
                }
            }
        }

        current_var = var_next_layer;

    }


    return BDD(root, this->layers);
}

NextState BDDCreator::compute_next_state(int var, bool value, BDDNode* current_node,
     std::unordered_map<int, int>& current_assignment, int next_layer)
{
    NextState result;
    result.status = NodeStatus::DEFAULT;

    for (const std::vector<int>& clause : current_node->get_unsat_clauses())
    {
        bool satisfied = false;
        bool has_unassigned_literal = false;

        for (const int literal : clause)
        {
            int current_var = std::abs(literal);

            if(current_assignment[current_var] == 0){
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
            if(!has_unassigned_literal){
                result.status = NodeStatus::UNSAT;
            }
            result.state.push_back(clause);
        }

        satisfied = false;
        has_unassigned_literal = false;
    }

    if(result.status == NodeStatus::UNSAT){
        return result;
    }

    if(result.state.empty()){
        result.status = NodeStatus::SAT;
        return result;
    }

    //check if node is infeasible

    for(std::vector<int>& clause : result.state){
        bool has_var_below_layer = false;

        for(const int lit : clause) {
            if(variable_below_layer(std::abs(lit), next_layer)){
                has_var_below_layer = true;
                break;
            }
        }

        if(!has_var_below_layer){
            result.status = NodeStatus::INFEASIBLE;
            return result;
        }
    }

    return result;
}

BDDNode* BDDCreator::obtain_equivalent_node(NextState& state, int next_layer)
{   
    std::vector<std::vector<int>>& current_state = state.state;

    std::string key = state_to_key(current_state);

    auto it = state_table[next_layer].find(key);

    if(it != state_table[next_layer].end()){

        BDDNode* node = it->second;

        if(state.status == node->get_status()){
            return node;
        }   
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
        for(const int lit : clause) {
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