#ifndef FLEXUL_TREE_HPP
#define FLEXUL_TREE_HPP

#include "token.hpp"
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

#endif
