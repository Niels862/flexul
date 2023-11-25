#ifndef FLEXUL_TREE_HPP
#define FLEXUL_TREE_HPP

#include "token.hpp"
#include "serializer.hpp"
#include "symbol.hpp"
#include <vector>
#include <unordered_map>

class Serializer;

class BaseNode {
public:
    BaseNode(uint32_t arity, Token token, std::vector<BaseNode *> children);
    BaseNode(Token token, std::vector<BaseNode *> children);
    virtual ~BaseNode();

    virtual bool is_lvalue() const;
    // First pass: collects symbols which can be referenced before declaration:
    // functions, global variables, definitions
    virtual void resolve_symbols_first_pass(
            Serializer &serializer, SymbolMap &current_scope);
    // Second pass: collects all other symbols and resolves occurences.
    virtual void resolve_symbols_second_pass(
            Serializer &serializer, SymbolMap &global_scope, 
            SymbolMap &enclosing_scope, SymbolMap &current_scope);
    virtual SymbolId register_symbol(
            Serializer &serializer, SymbolMap &scope, 
            StorageType storage_type, uint32_t value) const;
    virtual void serialize(Serializer &serializer) const = 0;
    virtual void serialize_load_address(Serializer &serializer) const;

    Token get_token() const;
    const std::vector<BaseNode *> &get_children() const;
    BaseNode *get_first() const;
    BaseNode *get_second() const;
    BaseNode *get_third() const;
    BaseNode *get_fourth() const;
    void set_id(SymbolId id);
    SymbolId get_id() const;
    static void print(BaseNode *node, std::string const labelPrefix = "", 
            std::string const branchPrefix = "");
private:
    BaseNode *get_nth(uint32_t n, std::string const &ordinal) const;
    Token token;
    std::vector<BaseNode *> children;
    SymbolId symbol_id;
};

class EmptyNode : public BaseNode {
public:
    EmptyNode(Token token, std::vector<BaseNode *> children);

    void serialize(Serializer &serializer) const override;
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

    bool is_lvalue() const override;
    void resolve_symbols_second_pass(
            Serializer &serializer, SymbolMap &global_scope, 
            SymbolMap &enclosing_scope, SymbolMap &current_scope) override;
    uint32_t register_symbol(
            Serializer &serializer, SymbolMap &scope, 
            StorageType storage_type, uint32_t value) const override;
    void serialize(Serializer &serializer) const override;
    void serialize_load_address(Serializer &serializer) const override;
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

class IfElseNode : public BaseNode {
public:
    IfElseNode(Token token, std::vector<BaseNode *> children);

    void serialize(Serializer &serializer) const override;
};

class CallNode : public BaseNode {
public:
    CallNode(Token token, std::vector<BaseNode *> children);

    void serialize(Serializer &serializer) const override;
};

// Block without attached scope, scope managed by outside node:
// primitve collection of statements like function body or file body
class BlockNode : public BaseNode {
public:
    BlockNode(Token token, std::vector<BaseNode *> children);

    void resolve_symbols_first_pass(
            Serializer &serializer, SymbolMap &symbol_map) override;
    void serialize(Serializer &serializer) const override;
private:
    SymbolMap scope_map;
};

// Block which introduces a new scope: statement blocks
class ScopedBlockNode : public BaseNode {
public:
    ScopedBlockNode(Token token, std::vector<BaseNode *> children);

    void serialize(Serializer &serializer) const override;
private:
    SymbolMap scope_map;
};

class FunctionNode : public BaseNode {
public:
    FunctionNode(Token token, std::vector<BaseNode *> children);

    void resolve_symbols_first_pass(
            Serializer &serializer, SymbolMap &symbol_map) override;
    void resolve_symbols_second_pass(
            Serializer &serializer, SymbolMap &global_scope, 
            SymbolMap &enclosing_scope, SymbolMap &current_scope) override;
    void serialize(Serializer &serializer) const override;
};

class LambdaNode : public BaseNode {
public:
    LambdaNode(Token token, std::vector<BaseNode *> children);

    void serialize(Serializer &serializer) const override;
};

class ExpressionListNode : public BaseNode {
public:
    ExpressionListNode(Token token, std::vector<BaseNode *> children);

    void serialize(Serializer &serializer) const override;
};

class IfNode : public BaseNode {
public:
    IfNode(Token token, std::vector<BaseNode *> children);

    void serialize(Serializer &serializer) const override;
};

class ForLoopNode : public BaseNode {
public:
    ForLoopNode(Token token, std::vector<BaseNode *> children);

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
