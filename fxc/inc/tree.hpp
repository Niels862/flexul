#ifndef FLEXUL_TREE_HPP
#define FLEXUL_TREE_HPP

#include "token.hpp"
#include "intermediate.hpp"
#include <vector>
#include <unordered_map>

using RenameMap = std::unordered_map<std::string, std::string>;

class Intermediate;

class BaseNode {
public:
    BaseNode(Token token, std::vector<BaseNode *> children);
    ~BaseNode();
    static void print(BaseNode *node, std::string const labelPrefix = "", 
            std::string const branchPrefix = "");
private:
    Token token;
    std::vector<BaseNode *> children;
};

class IntLitNode : public BaseNode {
public:
    IntLitNode(Token token);
private:
    int32_t value;
};

class VariableNode : public BaseNode {
public:
    VariableNode(Token token);
};

class UnaryNode : public BaseNode {
public:
    UnaryNode(Token op, BaseNode *child);
};

class BinaryNode : public BaseNode {
public:
    BinaryNode(Token op, BaseNode *left, BaseNode *right);
};

class BlockNode : public BaseNode {
public:
    BlockNode(std::vector<BaseNode *> children);
};

class Node {
public:
    Node();
    Node(Token token, int arity, std::vector<Node *> children);
    ~Node();
    void prepare(Intermediate intermediate);
    void translate(Intermediate &intermediate) const;
    Token get_token() const;
    std::vector<Node *> get_children() const;
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
    void register_function(Intermediate &intermediate) const;
    void register_declarations(Intermediate &intermediate, 
            uint32_t addr) const;

    Token token;
    int arity;
    std::vector<Node *> children;
};

#endif
