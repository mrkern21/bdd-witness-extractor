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

void BDD::traversePreOrder(const BDDNode* node, std::string& result, std::unordered_set<const BDDNode*>& visited) const {

    if(node == nullptr) return;

    if(visited.count(node) > 0) return;

    visited.insert(node);

    result += "var: " + std::to_string(node->getVar()) + " ";
    result += "layer: " + std::to_string(node->getLayer());
    result += "\n";

    traversePreOrder(node->getLow(), result, visited);
    traversePreOrder(node->getHigh(), result, visited);

}

std::string BDD::toString() const {
    std::string result;
    std::unordered_set<const BDDNode*> visited;

    traversePreOrder(this->root, result, visited);

    return result;
}