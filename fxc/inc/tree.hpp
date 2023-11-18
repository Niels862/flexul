#ifndef FLEXUL_TREE_HPP
#define FLEXUL_TREE_HPP

#include "token.hpp"
#include "serializer.hpp"
#include <vector>
#include <unordered_map>

using RenameMap = std::unordered_map<std::string, std::string>;

class Serializer;

class BaseNode {
public:
    BaseNode(uint32_t arity, Token token, std::vector<BaseNode *> children);
    BaseNode(Token token, std::vector<BaseNode *> children);
    virtual ~BaseNode();
    virtual void serialize(Serializer &serializer) const = 0;
    Token get_token() const;
    const std::vector<BaseNode *> &get_children() const;
    BaseNode *get_first() const;
    BaseNode *get_second() const;
    BaseNode *get_third() const;
    static void print(BaseNode *node, std::string const labelPrefix = "", 
            std::string const branchPrefix = "");
private:
    BaseNode *get_nth(uint32_t n, std::string const &ordinal) const;
    Token token;
    std::vector<BaseNode *> children;
};

class IntLitNode : public BaseNode {
public:
    IntLitNode(Token token, std::vector<BaseNode *> children);
    void serialize(Serializer &serializer) const;
private:
    int32_t value;
};

class VariableNode : public BaseNode {
public:
    VariableNode(Token token, std::vector<BaseNode *> children);
    void serialize(Serializer &serializer) const;
};

class UnaryNode : public BaseNode {
public:
    UnaryNode(Token token, std::vector<BaseNode *> children);
    void serialize(Serializer &serializer) const;
};

class BinaryNode : public BaseNode {
public:
    BinaryNode(Token token, std::vector<BaseNode *> children);
    void serialize(Serializer &serializer) const;
};

class BlockNode : public BaseNode {
public:
    BlockNode(Token token, std::vector<BaseNode *> children);
    void serialize(Serializer &serializer) const;
};

class FunctionNode : public BaseNode {
public:
    FunctionNode(Token token, std::vector<BaseNode *> children);
    void serialize(Serializer &serializer) const;
};

class ReturnNode : public BaseNode {
public:
    ReturnNode(Token token, std::vector<BaseNode *> children);
    void serialize(Serializer &serializer) const;
};

class ExpressionStatementNode : public BaseNode {
public:
    ExpressionStatementNode(Token token, std::vector<BaseNode *> children);
    void serialize(Serializer &serializer) const;
};

#endif
