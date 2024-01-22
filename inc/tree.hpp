#ifndef FLEXUL_TREE_HPP
#define FLEXUL_TREE_HPP

#include "token.hpp"
#include "serializer.hpp"
#include "symbol.hpp"
#include <vector>
#include <unordered_map>

class Serializer;

template <typename T>
struct Maybe {
    T value;
    bool null;
};

class BaseNode {
public:
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
    virtual void serialize(Serializer &serializer) const = 0;
    virtual void serialize_load_address(Serializer &serializer) const;
    virtual Maybe<uint32_t> get_constant_value() const;

    virtual std::string get_label() const;
    Token get_token() const;
    const std::vector<BaseNode *> &get_children() const;
    BaseNode *get(size_t n) const;
    void set_id(SymbolId id);
    SymbolId get_id() const;

    static void print(BaseNode *node, std::string const labelPrefix = "", 
            std::string const branchPrefix = "");
private:
    Token token;
    std::vector<BaseNode *> children;
    SymbolId symbol_id;
};

class EmptyNode : public BaseNode {
public:
    EmptyNode();

    void serialize(Serializer &serializer) const override;
};

class IntLitNode : public BaseNode {
public:
    IntLitNode(Token token);

    void serialize(Serializer &serializer) const override;
    Maybe<uint32_t> get_constant_value() const;
private:
    uint32_t value;
};

class VariableNode : public BaseNode {
public:
    VariableNode(Token token);

    bool is_lvalue() const override;
    void resolve_symbols_second_pass(
            Serializer &serializer, SymbolMap &global_scope, 
            SymbolMap &enclosing_scope, SymbolMap &current_scope) override;
    void serialize(Serializer &serializer) const override;
    void serialize_load_address(Serializer &serializer) const override;
};

class UnaryNode : public BaseNode {
public:
    UnaryNode(Token token, BaseNode *child);

    bool is_lvalue() const override;
    void serialize(Serializer &serializer) const override;
    void serialize_load_address(Serializer &serializer) const override;
private:
    size_t const Child = 0;
};

class BinaryNode : public BaseNode {
public:
    BinaryNode(Token token, BaseNode *left, BaseNode *right);

    void serialize(Serializer &serializer) const override;
private:
    size_t const Left = 0, Right = 1;
};

class IfElseNode : public BaseNode {
public:
    IfElseNode(Token token, BaseNode *cond, BaseNode *case_true, 
            BaseNode *case_false);

    void serialize(Serializer &serializer) const override;
private:
    size_t const Cond = 0, CaseTrue = 1, CaseFalse = 2;
};

class CallNode : public BaseNode {
public:
    CallNode(BaseNode *func, BaseNode *args);

    void serialize(Serializer &serializer) const override;
private:
    size_t const Func = 0, Args = 1;
};

class SubscriptNode : public BaseNode {
public:
    SubscriptNode(BaseNode *array, BaseNode *subscript);

    bool is_lvalue() const override;
    void serialize(Serializer &serializer) const override;
    void serialize_load_address(Serializer &serializer) const override;
private:
    size_t const Array = 0, Subscript = 1;
};

// Block without attached scope, scope managed by outside node:
// primitve collection of statements like function body or file body
class BlockNode : public BaseNode {
public:
    BlockNode(std::vector<BaseNode *> children);

    void resolve_symbols_first_pass(
            Serializer &serializer, SymbolMap &symbol_map) override;
    void serialize(Serializer &serializer) const override;
private:
    SymbolMap scope_map;
};

// Block which introduces a new scope: statement blocks
class ScopedBlockNode : public BaseNode {
public:
    ScopedBlockNode(std::vector<BaseNode *> children);

    void resolve_symbols_second_pass(
            Serializer &serializer, SymbolMap &global_scope, 
            SymbolMap &enclosing_scope, SymbolMap &current_scope) override;
    void serialize(Serializer &serializer) const override;
private:
    SymbolMap scope_map;
};

class CallableNode : public BaseNode {
public:
    CallableNode(Token token, Token name, std::vector<Token> params, 
            BaseNode *body);
    
    bool is_matching_call(BaseNode *params) const;
    virtual void serialize_call(Serializer &serializer, 
            BaseNode *params) const = 0;

    Token const &get_ident() const;
    std::vector<Token> const &get_params() const;
    uint32_t get_n_params() const;

    size_t const Body = 0;
private:
    Token ident;
    std::vector<Token> params;
};

class FunctionNode : public CallableNode {
public:
    FunctionNode(Token token, Token ident, std::vector<Token> params, 
            BaseNode *body);

    void resolve_symbols_first_pass(
            Serializer &serializer, SymbolMap &symbol_map) override;
    void resolve_symbols_second_pass(
            Serializer &serializer, SymbolMap &global_scope, 
            SymbolMap &enclosing_scope, SymbolMap &current_scope) override;
    void serialize(Serializer &serializer) const override;
    void serialize_call(Serializer &serializer, BaseNode *params) const;

    std::string get_label() const;

private:
    uint32_t frame_size;
};

class InlineNode : public CallableNode {
public:
    InlineNode(Token token, Token ident, std::vector<Token> params, 
            BaseNode *body);

    void resolve_symbols_first_pass(
            Serializer &serializer, SymbolMap &symbol_map) override;
    void resolve_symbols_second_pass(
            Serializer &serializer, SymbolMap &global_scope, 
            SymbolMap &enclosing_scope, SymbolMap &current_scope) override;
    void serialize(Serializer &serializer) const override;
    void serialize_call(Serializer &serializer, BaseNode *params) const;

    std::string get_label() const;
};

class LambdaNode : public BaseNode {
public:
    LambdaNode(Token token, std::vector<Token>, BaseNode *body);

    void resolve_symbols_second_pass(
            Serializer &serializer, SymbolMap &global_scope, 
            SymbolMap &enclosing_scope, SymbolMap &current_scope) override;
    void serialize(Serializer &serializer) const override;

    std::string get_label() const;
private:
    size_t const Body = 0;

    std::vector<Token> params;
};

class ExpressionListNode : public BaseNode {
public:
    ExpressionListNode(Token token, std::vector<BaseNode *> children);

    void serialize(Serializer &serializer) const override;
};

class IfNode : public BaseNode {
public:
    IfNode(Token token, BaseNode *cond, BaseNode *case_true);

    void serialize(Serializer &serializer) const override;
private:
    size_t const Cond = 0, CaseTrue = 1;
};

class ForLoopNode : public BaseNode {
public:
    ForLoopNode(Token token, BaseNode *init, BaseNode *cond, BaseNode *post, 
            BaseNode *body);

    void serialize(Serializer &serializer) const override;
private:
    size_t const Init = 0, Cond = 1, Post = 2, Body = 3;
};

class ReturnNode : public BaseNode {
public:
    ReturnNode(Token token, BaseNode *child);

    void serialize(Serializer &serializer) const override;
private:
    size_t const RetValue = 0;
};

class DeclarationNode : public BaseNode {
public:
    DeclarationNode(Token token, Token ident, BaseNode *size, 
            BaseNode *init_value);

    void resolve_symbols_first_pass(
            Serializer &serializer, SymbolMap &current_scope);
    void resolve_symbols_second_pass(
            Serializer &serializer, SymbolMap &global_scope, 
            SymbolMap &enclosing_scope, SymbolMap &current_scope) override;
    void serialize(Serializer &serializer) const override;

    std::string get_label() const;
private:
    size_t const Size = 0, InitValue = 1;

    uint32_t get_size() const;

    Token ident;
};

class ExpressionStatementNode : public BaseNode {
public:
    ExpressionStatementNode(BaseNode *child);
    
    void serialize(Serializer &serializer) const override;
private:
    size_t const Expr = 0;
};

class AliasNode : public BaseNode {
public:
    AliasNode(Token token, Token alias, Token source);

    void resolve_symbols_second_pass(
            Serializer &serializer, SymbolMap &global_scope, 
            SymbolMap &enclosing_scope, SymbolMap &current_scope) override;
    void serialize(Serializer &serializer) const override;

    std::string get_label() const;
private:
    Token alias;
    Token source;
};

#endif
