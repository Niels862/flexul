#ifndef FLEXUL_TREE_HPP
#define FLEXUL_TREE_HPP

#include "token.hpp"
#include "serializer.hpp"
#include "symbol.hpp"
#include <vector>
#include <unordered_map>
#include <optional>


class Serializer; 

class TypeNode;

struct CallableSignature {
    std::vector<Token> params;
    std::vector<TypeNode *> param_types;
    TypeNode *return_type;
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
    virtual std::optional<uint32_t> get_constant_value() const;

    virtual std::string label() const;
    Token token() const;
    virtual std::vector<BaseNode *> const &children() const;
    BaseNode *get(size_t n) const;
    void set_id(SymbolId id);
    SymbolId id() const;

    static void print(BaseNode *node,
            std::string const &label_prefix = "", 
            std::string const &branch_prefix = "");
private:
    Token m_token;
    std::vector<BaseNode *> m_children;
    TypeNode *m_type;
    SymbolId m_id;
};

class TypeNode : public BaseNode {
public:
    TypeNode(Token token, std::vector<BaseNode *> children);

    void serialize(Serializer &serializer) const override;
};

class NamedTypeNode : public TypeNode {
public:
    NamedTypeNode(Token ident);
private:
    Token m_ident;
};

class CallableTypeNode : public TypeNode {
public:
    CallableTypeNode(TypeNode *param_types, TypeNode *return_type);
private:
    std::vector<TypeNode *> m_param_types;
    TypeNode *return_type;
};

class EmptyNode : public BaseNode {
public:
    EmptyNode();

    void serialize(Serializer &serializer) const override;
};

class LinkNode : public BaseNode {
public:
    LinkNode(BaseNode *link);

    void serialize(Serializer &serializer) const override;

    std::vector<BaseNode *> const &children() const override;
private:
    std::vector<BaseNode *> link;
    size_t const Link = 0;
};

class IntLitNode : public BaseNode {
public:
    IntLitNode(Token token);

    void serialize(Serializer &serializer) const override;
    std::optional<uint32_t> get_constant_value() const;
private:
    uint32_t m_value;
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

class AssignNode : public BaseNode {
public:
    AssignNode(Token token, BaseNode *target, BaseNode *expr);

    void serialize(Serializer &serializer) const override;
private:
    size_t const Target = 0, Expr = 1;
};

class AndNode : public BaseNode {
public:
    AndNode(Token token, BaseNode *left, BaseNode *right);

    void serialize(Serializer &serializer) const override;
private:
    size_t const Left = 0, Right = 1;
};

class OrNode : public BaseNode {
public:
    OrNode(Token token, BaseNode *left, BaseNode *right);

    void serialize(Serializer &serializer) const override;
private:
    size_t const Left = 0, Right = 1;
};

class AddressOfNode : public BaseNode {
public:
    AddressOfNode(Token token, BaseNode *operand);

    void serialize(Serializer &serializer) const override;
private:
    size_t const Operand = 0;
};

class DereferenceNode : public BaseNode {
public:
    DereferenceNode(Token token, BaseNode *operand);

    bool is_lvalue() const override;
    void serialize(Serializer &serializer) const override;
    void serialize_load_address(Serializer &serializer) const override;
private:
    size_t const Operand = 0;
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
    SymbolMap m_scope_map;
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
    SymbolMap m_scope_map;
};

class CallableNode : public BaseNode {
public:
    CallableNode(Token token, Token ident, 
            CallableSignature signature, BaseNode *body);
    
    bool is_matching_call(BaseNode *params) const;
    virtual void serialize_call(Serializer &serializer, 
            BaseNode *params) const = 0;

    Token const &ident() const;
    std::vector<Token> const &params() const;
    uint32_t n_params() const;

    size_t const Body = 0;
private:
    Token m_ident;
    CallableSignature m_signature;
};

class FunctionNode : public CallableNode {
public:
    FunctionNode(Token token, Token ident, 
            CallableSignature signature, BaseNode *body);

    void resolve_symbols_first_pass(
            Serializer &serializer, SymbolMap &symbol_map) override;
    void resolve_symbols_second_pass(
            Serializer &serializer, SymbolMap &global_scope, 
            SymbolMap &enclosing_scope, SymbolMap &current_scope) override;
    void serialize(Serializer &serializer) const override;
    void serialize_call(Serializer &serializer, 
            BaseNode *params) const override;

    std::string label() const;
private:
    uint32_t m_frame_size;
};

class InlineNode : public CallableNode {
public:
    InlineNode(Token token, Token ident, 
            CallableSignature signature, BaseNode *body);

    void resolve_symbols_first_pass(
            Serializer &serializer, SymbolMap &symbol_map) override;
    void resolve_symbols_second_pass(
            Serializer &serializer, SymbolMap &global_scope, 
            SymbolMap &enclosing_scope, SymbolMap &current_scope) override;
    void serialize(Serializer &serializer) const override;
    void serialize_call(Serializer &serializer, 
            BaseNode *params) const override;

    std::string get_label() const;
private:
    std::vector<SymbolId> m_param_ids;
};

class LambdaNode : public CallableNode {
public:
    LambdaNode(Token token, 
            CallableSignature signature, BaseNode *body);

    void resolve_symbols_second_pass(
            Serializer &serializer, SymbolMap &global_scope, 
            SymbolMap &enclosing_scope, SymbolMap &current_scope) override;
    void serialize(Serializer &serializer) const override;
    // Note: lambda call is serialized upon call to serialize,
    // body is also added as new code job
    void serialize_call(Serializer &serializer, 
            BaseNode *params) const override;

    std::string label() const;
};

class TypeDeclarationNode : public BaseNode {
public:
    TypeDeclarationNode(Token token, Token ident);

    void resolve_symbols_first_pass(
            Serializer &serializer, SymbolMap &symbol_map) override;
    void serialize(Serializer &serializer) const override;

    std::string label() const override;
private:
    Token m_ident;
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

class VarDeclarationNode : public BaseNode {
public:
    VarDeclarationNode(Token token, Token ident, BaseNode *size, 
            BaseNode *init_value);

    void resolve_symbols_first_pass(
            Serializer &serializer, SymbolMap &current_scope);
    void resolve_symbols_second_pass(
            Serializer &serializer, SymbolMap &global_scope, 
            SymbolMap &enclosing_scope, SymbolMap &current_scope) override;
    void serialize(Serializer &serializer) const override;

    std::string label() const override;
private:
    size_t const Size = 0, InitValue = 1;

    uint32_t declared_size() const;

    Token m_ident;
};

class ExpressionStatementNode : public BaseNode {
public:
    ExpressionStatementNode(BaseNode *child);
    
    void serialize(Serializer &serializer) const override;
private:
    size_t const Expr = 0;
};

#endif
