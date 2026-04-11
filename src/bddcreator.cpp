#include "bddcreator.hpp"

BDDCreator::BDDCreator(std::vector<std::vector<int>>& clauses,
                   std::queue<int>& varorder) : clauses(clauses), varorder(varorder) {
    this->clauses = clauses;
    this->varorder = varorder;
    this->num_of_layers = varorder.size();
}

//notice that there are n layers in the BDD where 'n' is the number of distinct variables used in the given clauses
//we obtain 'n' by the size of the 'varorder' list. 
BDD BDDCreator::createBDD(){
    int current_layer = 0;
    int current_var = varorder.front();
    varorder.pop();

    std::vector<BDDNode*> unsatNodes;
    BDDNode* root = new BDDNode(current_var, current_layer, this->clauses, this->clauses);
    layers[current_layer].push_back(root);
    

    //now create remaining layers
    for(int i = current_layer; i < num_of_layers; i++){
        int varNextLayer = varorder.front();
        for(BDDNode* node : layers[current_layer]){
            if(!node->isUnsat()){
                for (bool value : this->domain){
                    std::vector<std::vector<int>> newState = computeNextState(current_var, value, node);
                    std::vector<std::vector<int>> partialAssignment = computePartialAssignment(current_var, value);


                    BDDNode* equivalentNode = obtainEquivalentNode(newState, current_layer + 1);
                    
                    if(equivalentNode != nullptr){
                        if(value){
                            node->setHigh(equivalentNode);
                        }else{
                            node->setLow(equivalentNode);
                        }
                    }else{

                        BDDNode* newNode = new BDDNode(varNextLayer, current_layer + 1, newState, partialAssignment);
                        layers[current_layer + 1].push_back(newNode);

                        if(value){
                            newNode->setHigh(newNode);
                        }else{
                            newNode->setLow(newNode);
                        }
                        
                    }

                }
            }
        }

        current_layer++;

        //remove nextLayerVar at the end
        varorder.pop();
    }
    

    return BDD(root, unsatNodes);
}

std::vector<std::vector<int>> BDDCreator::computeNextState(int var, bool value, BDDNode* currentNode) {
    std::vector<std::vector<int>> newState;
    std::vector<std::vector<int>> currentState = currentNode->getUnsatClauses();
    for (std::vector<int> clause : currentState) {
        bool satisfied = false;
        for(int literal : clause) {
            if(literal > 0){
                if(literal == var && value){
                    satisfied = true;
                    break;
                }
            }else {
                if(literal == -var && !value){
                    satisfied = true;
                    break;
                }
            }
        }
        if(!satisfied){
            newState.push_back(clause);
        }
        satisfied = false;
    }
    
    return newState;
}

BDDNode* BDDCreator::obtainEquivalentNode(std::vector<std::vector<int>> currentState, int nextLayer){
    return nullptr;
}

std::vector<std::vector<int>> BDDCreator::computePartialAssignment(int var, bool value){

}

void BDDCreator::checkUnsat(void) {

}

void BDDCreator::checkInfeasible(void) {

}