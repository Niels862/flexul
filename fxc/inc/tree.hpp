#include "token.hpp"
#include <vector>

#ifndef FLEXUL_TREE_HPP
#define FLEXUL_TREE_HPP

class Node {
public:
    Node();
    Node(Token token, Node *a, Node *b, Node *c);
private:
    Token token;
    Node *a;
    Node *b;
    Node *c;
};

class Tree {
public:
    Tree();
    Node *add_node(Token token, Node *a = nullptr, Node *b = nullptr, Node *c = nullptr);
private:

    std::vector<Node> nodes;
    Node *root;
};

#endif
