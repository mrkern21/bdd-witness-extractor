#include "bdd.hpp"

BDD::BDD(BDDNode* root, std::vector<BDDNode*>& unsatNodes) : unsatNodes(unsatNodes) {
    this->root = root;
}

BDD::~BDD() {
    delete this->root;
}

std::vector<std::vector<int>> BDD::generateWitnessClauses() {
    std::vector<std::vector<int>> result = {};
    return result;
}

std::string BDD::toString() const {
    std::string result;

    int var = root->getVar();
    int layer = root->getLayer();
    
    result.append("var: ");
    result.append(std::to_string(var));
    result.append(" layer: ");
    result.append(std::to_string(layer));
    result.append("\n");
    

    return result;
}