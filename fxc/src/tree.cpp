#include "tree.hpp"

Node::Node()
        : token(), a(nullptr), b(nullptr), c(nullptr) {}

Node::Node(Token token, Node *a, Node *b, Node *c) 
        : token(token), a(a), b(b), c(c) {}

Tree::Tree() {

}

Node *Tree::add_node(Token token, Node *a, Node *b, Node *c) {
    Node node(token, a, b, c);
    nodes.push_back(node);
    return &nodes[nodes.size() - 1];
}
