#include "token.hpp"
#include "intermediate.hpp"
#include <vector>
#include <unordered_map>

#ifndef FLEXUL_TREE_HPP
#define FLEXUL_TREE_HPP

using RenameMap = std::unordered_map<std::string, std::string>;

class Node {
public:
    Node();
    Node(Token token, int arity, std::vector<Node *> children);
    ~Node();
    void rename();
    void translate(Intermediate &intermediate) const;
    static void print(Node *node, std::string const labelPrefix = "", 
            std::string const branchPrefix = "");
private:
    void assign_new_name(RenameMap &scope_map, size_t &i);
    void rename(RenameMap &scope_map, size_t &i);

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
