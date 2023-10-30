#include "tree.hpp"
#include <iostream>

Node::Node()
        : token(), children() {}

Node::Node(Token token, std::vector<Node *> children) 
        : token(token), children(children) {}

Node::~Node() {
    for (Node * const node : children) {
        delete node;
    }
}


void Node::print(Node *node, std::string const labelPrefix, 
        std::string const branchPrefix) {
    size_t i;
    if (node == nullptr) {
        std::cerr << labelPrefix << "NULL" << std::endl;
        return;
    }
    std::cerr << labelPrefix << node->token.get_data() << std::endl;
    if (node->children.empty()) {
        return;
    }
    for (i = 0; i < node->children.size() - 1; i++) {
        Node::print(node->children[i], 
                branchPrefix + "\u251C\u2500\u2500\u2500", 
                branchPrefix + "\u2502   ");
    }
    Node::print(node->children[i], 
            branchPrefix + "\u2514\u2500\u2500\u2500", 
            branchPrefix + "    ");
}
