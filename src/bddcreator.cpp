#include "bddcreator.hpp"

BDDCreator::BDDCreator(std::vector<std::vector<int>>& clauses,
                       std::queue<int>& varorder) : clauses(clauses), varorder(varorder),
                       num_of_layers(static_cast<int>(varorder.size())), layers(num_of_layers + 1)
{
}

// notice that there are n layers in the BDD where 'n' is the number of distinct variables used in the given clauses
// we obtain 'n' by the size of the 'varorder' list.
BDD BDDCreator::createBDD(void)
{
    int current_layer = 0;
    int current_var = varorder.front();
    varorder.pop();

    std::vector<BDDNode*> unsatNodes;
    BDDNode* root = new BDDNode(current_var, current_layer, this->clauses);
    layers[current_layer].push_back(root);

    // now create remaining layers
    for (int i = current_layer; i < num_of_layers - 1; i++)
    {
        int varNextLayer = varorder.front();
        int next_layer = current_layer + 1;
        for (BDDNode* node : layers[current_layer])
        {
            if (node->isUnsat()) continue;
            
            for (bool value : this->domain)
            {
                std::vector<std::vector<int>> newState = computeNextState(current_var, value, node);

                BDDNode* equivalentNode = obtainEquivalentNode(newState, next_layer);

                if (equivalentNode != nullptr)
                {
                    if (value) node->setHigh(equivalentNode);
                    else node->setLow(equivalentNode);
                }
                else
                {
                    BDDNode* newNode = new BDDNode(varNextLayer, next_layer, newState);
                    layers[next_layer].push_back(newNode);
                    if (value) node->setHigh(newNode);
                    else node->setLow(newNode);
                }
            }
        }

        current_layer++;
        varorder.pop(); 
    }


    return BDD(root, unsatNodes);
}

std::vector<std::vector<int>> BDDCreator::computeNextState(int var, bool value, BDDNode *currentNode)
{
    std::vector<std::vector<int>> newState;
    for (const std::vector<int>& clause : currentNode->getUnsatClauses())
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
            newState.push_back(clause);
        }
        satisfied = false;
    }

    return newState;
}

BDDNode *BDDCreator::obtainEquivalentNode(std::vector<std::vector<int>> &currentState, int nextLayer)
{
    return nullptr;
}

void BDDCreator::checkUnsat(void)
{
}

void BDDCreator::checkInfeasible(void)
{
}