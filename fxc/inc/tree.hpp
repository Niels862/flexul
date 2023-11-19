#ifndef FLEXUL_TREE_HPP
#define FLEXUL_TREE_HPP

#include "token.hpp"
#include "serializer.hpp"
#include <vector>
#include <unordered_map>

using SymbolMap = std::unordered_map<std::string, uint32_t>;

class Serializer;

class BaseNode {
public:
    BaseNode(uint32_t arity, Token token, std::vector<BaseNode *> children);
    BaseNode(Token token, std::vector<BaseNode *> children);
    virtual ~BaseNode();

    virtual void resolve_symbols_first_pass(
            Serializer &serializer, SymbolMap &symbol_map);
    virtual void resolve_symbols_second_pass(
            Serializer &serializer, SymbolMap &symbol_map);
    virtual uint32_t register_symbol(
            Serializer &serializer, SymbolMap &symbol_map) const;
    virtual void serialize(Serializer &serializer) const = 0;
    
    Token get_token() const;
    const std::vector<BaseNode *> &get_children() const;
    BaseNode *get_first() const;
    BaseNode *get_second() const;
    BaseNode *get_third() const;
    void set_id(uint32_t id);
    uint32_t get_id() const;
    static void print(BaseNode *node, std::string const labelPrefix = "", 
            std::string const branchPrefix = "");
private:
    BaseNode *get_nth(uint32_t n, std::string const &ordinal) const;
    Token token;
    std::vector<BaseNode *> children;
    uint32_t symbol_id;
};

class IntLitNode : public BaseNode {
public:
    IntLitNode(Token token, std::vector<BaseNode *> children);

    void serialize(Serializer &serializer) const override;
private:
    int32_t value;
};

class VariableNode : public BaseNode {
public:
    VariableNode(Token token, std::vector<BaseNode *> children);

    void resolve_symbols_second_pass(
            Serializer &serializer, SymbolMap &symbol_map) override;
    uint32_t register_symbol(
            Serializer &serializer, SymbolMap &symbol_map) const override;
    void serialize(Serializer &serializer) const override;
};

class UnaryNode : public BaseNode {
public:
    UnaryNode(Token token, std::vector<BaseNode *> children);

    void serialize(Serializer &serializer) const override;
};

class BinaryNode : public BaseNode {
public:
    BinaryNode(Token token, std::vector<BaseNode *> children);

    void serialize(Serializer &serializer) const override;
};

class CallNode : public BaseNode {
public:
    CallNode(Token token, std::vector<BaseNode *> children);

    void serialize(Serializer &serializer) const override;
};

class BlockNode : public BaseNode {
public:
    BlockNode(Token token, std::vector<BaseNode *> children);

    void resolve_symbols_first_pass(
            Serializer &serializer, SymbolMap &symbol_map) override;
    void serialize(Serializer &serializer) const override;
};

class FunctionNode : public BaseNode {
public:
    FunctionNode(Token token, std::vector<BaseNode *> children);

    void resolve_symbols_first_pass(
            Serializer &serializer, SymbolMap &symbol_map) override;
    void resolve_symbols_second_pass(
            Serializer &serializer, SymbolMap &symbol_map) override;
    void serialize(Serializer &serializer) const override;
};

class ExpressionListNode : public BaseNode {
public:
    ExpressionListNode(Token token, std::vector<BaseNode *> children);

    void serialize(Serializer &serializer) const override;
};

class ReturnNode : public BaseNode {
public:
    ReturnNode(Token token, std::vector<BaseNode *> children);

    void serialize(Serializer &serializer) const override;
};

class ExpressionStatementNode : public BaseNode {
public:
    ExpressionStatementNode(Token token, std::vector<BaseNode *> children);
    
    void serialize(Serializer &serializer) const override;
};

#endif
