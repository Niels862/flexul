#include "token.hpp"
#include <vector>

#ifndef FLEXUL_TREE_HPP
#define FLEXUL_TREE_HPP

class Node {
public:
    Node();
    Node(Token token, std::vector<Node *> children);
    ~Node();
    static void print(Node *node, std::string const labelPrefix = "", 
            std::string const branchPrefix = "");
private:
    Token token;
    std::vector<Node *> children;
};

#endif
