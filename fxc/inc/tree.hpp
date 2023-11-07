#include "token.hpp"
#include "intermediate.hpp"
#include <vector>
#include <unordered_map>

#ifndef FLEXUL_TREE_HPP
#define FLEXUL_TREE_HPP

class Node {
public:
    Node();
    Node(Token token, int arity, std::vector<Node *> children);
    ~Node();
    void translate(Intermediate &intermediate) const;
    static void print(Node *node, std::string const labelPrefix = "", 
            std::string const branchPrefix = "");
private:
    void translate_dispatch(Intermediate &intermediate) const;
    void translate_value(Intermediate &intermediate) const;
    void translate_unary(Intermediate &intermediate) const;
    void translate_binary(Intermediate &intermediate) const;
    void translate_ternary(Intermediate &intermediate) const;

    Token token;
    int arity;
    std::vector<Node *> children;
};

#endif
