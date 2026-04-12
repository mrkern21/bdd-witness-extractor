#include "bddcreator.hpp"

BDDCreator::BDDCreator(std::vector<std::vector<int>>& clauses,
                       std::queue<int>& varorder) : clauses(clauses), varorder(varorder),
                       num_of_layers(static_cast<int>(varorder.size())), layers(num_of_layers + 1)
{
}

// notice that there are n layers in the BDD where 'n' is the number of distinct variables used in the given clauses
// we obtain 'n' by the size of the 'varorder' list.
BDD BDDCreator::create_bdd(void)
{
    int current_layer = 0;
    int current_var = varorder.front();
    varorder.pop();

    std::vector<BDDNode*> unsat_nodes;
    BDDNode* root = new BDDNode(current_var, current_layer, this->clauses);
    layers[current_layer].push_back(root);

    // now create remaining layers
    for (int i = current_layer; i < num_of_layers - 1; i++)
    {
        int var_next_layer = varorder.front();
        int next_layer = current_layer + 1;
        for (BDDNode* node : layers[current_layer])
        {
            if (node->is_unsat()) continue;
            
            for (bool value : this->domain)
            {
                std::vector<std::vector<int>> newState = compute_next_state(current_var, value, node);

                BDDNode* equivalent_node = obtain_equivalent_node(newState, next_layer);

                if (equivalent_node != nullptr)
                {
                    if (value) node->set_high(equivalent_node);
                    else node->set_low(equivalent_node);
                }
                else
                {
                    BDDNode* new_node = new BDDNode(var_next_layer, next_layer, newState);
                    layers[next_layer].push_back(new_node);
                    if (value) node->set_high(new_node);
                    else node->set_low(new_node);
                }
            }
        }

        current_layer++;
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

BDDNode *BDDCreator::obtain_equivalent_node(std::vector<std::vector<int>> &current_state, int next_layer)
{
    return nullptr;
}