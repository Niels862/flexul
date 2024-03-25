#ifndef FLEXUL_TREE_HPP
#define FLEXUL_TREE_HPP

#include "token.hpp"
#include "serializer.hpp"
#include "symbol.hpp"
#include <vector>
#include <unordered_map>
#include <optional>
#include <memory>

class Serializer; 

class TreePrinter;

class TypeNode;

class ExpressionListNode;

class BaseNode {
public:
    BaseNode(Token token);
    virtual ~BaseNode();

    virtual bool is_lvalue() const;
    // First pass: collects symbols which can be referenced before declaration:
    // functions, global variables, definitions
    virtual void resolve_globals(Serializer &serializer, SymbolMap &current);
    // Second pass: collects all other symbols and resolves occurences.
    virtual void resolve_locals(Serializer &serializer, ScopeTracker &scopes);
    virtual void serialize(Serializer &serializer) const = 0;
    virtual void serialize_load_address(Serializer &serializer) const;
    virtual std::optional<uint32_t> get_constant_value() const;

    virtual void print(TreePrinter &printer) const = 0;

    virtual std::string label() const;
    Token token() const;
    void set_id(SymbolId id);
    SymbolId id() const;
private:
    Token m_token;
    std::unique_ptr<TypeNode> m_type;
    SymbolId m_id;
};

class ExpressionNode : public BaseNode {
public:
    ExpressionNode(Token token);
};

class StatementNode : public BaseNode {
public:
    StatementNode(Token token);
};

class TypeNode : public BaseNode {
public:
    TypeNode(Token token);

    void serialize(Serializer &serializer) const override;
};

class UnaryExpressionNode : public ExpressionNode {
public:
    UnaryExpressionNode(Token token, std::unique_ptr<BaseNode> operand);

    void resolve_locals(Serializer &serializer, ScopeTracker &scopes) override;

    void print(TreePrinter &printer) const override;
protected:
    // todo BaseNode -> ExpressionNode
    std::unique_ptr<BaseNode> m_operand;
};

class AddressOfNode : public UnaryExpressionNode {
public:
    AddressOfNode(Token token, std::unique_ptr<BaseNode> operand);

    void serialize(Serializer &serializer) const override;
};

class DereferenceNode : public UnaryExpressionNode {
public:
    DereferenceNode(Token token, std::unique_ptr<BaseNode> operand);

    bool is_lvalue() const override;
    void serialize(Serializer &serializer) const override;
    void serialize_load_address(Serializer &serializer) const override;
};

class BinaryExpressionNode : public ExpressionNode {
public:
    BinaryExpressionNode(Token token, std::unique_ptr<BaseNode> left, 
            std::unique_ptr<BaseNode> right);

    void resolve_locals(Serializer &serializer, ScopeTracker &scopes) override;

    void print(TreePrinter &printer) const override;
protected:
    std::unique_ptr<BaseNode> m_left;
    std::unique_ptr<BaseNode> m_right;
};

class AssignNode : public BinaryExpressionNode {
public:
    AssignNode(Token token, std::unique_ptr<BaseNode> left, 
            std::unique_ptr<BaseNode> right);

    void serialize(Serializer &serializer) const override;
};

class AndNode : public BinaryExpressionNode {
public:
    AndNode(Token token, std::unique_ptr<BaseNode> left, 
            std::unique_ptr<BaseNode> right);

    void serialize(Serializer &serializer) const override;
};

class OrNode : public BinaryExpressionNode {
public:
    OrNode(Token token, std::unique_ptr<BaseNode> left, 
            std::unique_ptr<BaseNode> right);

    void serialize(Serializer &serializer) const override;
};

class NamedTypeNode : public TypeNode {
public:
    NamedTypeNode(Token ident);

    void print(TreePrinter &printer) const override;
};

class TypeListNode : public TypeNode {
public:
    TypeListNode(std::vector<std::unique_ptr<TypeNode>> type_list);

    void print(TreePrinter &printer) const override;
private:
    std::vector<std::unique_ptr<TypeNode>> m_type_list;
};

class CallableTypeNode : public TypeNode {
public:
    CallableTypeNode(Token token, std::unique_ptr<TypeListNode> param_types, 
            std::unique_ptr<TypeNode> return_type);
    
    void print(TreePrinter &printer) const override;
private:
    std::unique_ptr<TypeListNode> m_param_types;
    std::unique_ptr<TypeNode> m_return_type;
};

struct CallableSignature {
    CallableSignature(std::vector<Token> params, 
            std::unique_ptr<CallableTypeNode> type);

    std::vector<Token> params;
    std::unique_ptr<CallableTypeNode> type;
};

class EmptyNode : public BaseNode {
public:
    EmptyNode();

    void serialize(Serializer &serializer) const override;

    void print(TreePrinter &printer) const override;
};

class IntLitNode : public BaseNode {
public:
    IntLitNode(Token token);

    void serialize(Serializer &serializer) const override;
    std::optional<uint32_t> get_constant_value() const;

    void print(TreePrinter &printer) const override;
private:
    uint32_t m_value;
};

class VariableNode : public BaseNode {
public:
    VariableNode(Token token);

    bool is_lvalue() const override;
    void resolve_locals(Serializer &serializer, ScopeTracker &scopes) override;
    void serialize(Serializer &serializer) const override;
    void serialize_load_address(Serializer &serializer) const override;

    void print(TreePrinter &printer) const override;
};

class IfElseNode : public BaseNode {
public:
    IfElseNode(Token token, std::unique_ptr<BaseNode> cond, 
            std::unique_ptr<BaseNode> case_true, 
            std::unique_ptr<BaseNode> case_false);

    void resolve_locals(Serializer &serializer, ScopeTracker &scopes) override;
    void serialize(Serializer &serializer) const override;

    void print(TreePrinter &printer) const override;
private:
    std::unique_ptr<BaseNode> m_cond;
    std::unique_ptr<BaseNode> m_case_true;
    std::unique_ptr<BaseNode> m_case_false;
};

class CallNode : public BaseNode {
public:
    CallNode(std::unique_ptr<BaseNode> func, 
            std::unique_ptr<ExpressionListNode> args);

    static std::unique_ptr<BaseNode> make_call(Token ident, 
            std::vector<std::unique_ptr<BaseNode>> params);
    static std::unique_ptr<BaseNode> make_unary_call(Token ident, 
            std::unique_ptr<BaseNode> param);
    static std::unique_ptr<BaseNode> make_binary_call(Token ident, 
            std::unique_ptr<BaseNode> left, 
            std::unique_ptr<BaseNode> right);

    void resolve_locals(Serializer &serializer, ScopeTracker &scopes) override;
    void serialize(Serializer &serializer) const override;

    void print(TreePrinter &printer) const override;
private:
    std::unique_ptr<BaseNode> m_func;
    std::unique_ptr<ExpressionListNode> m_args;
};

class SubscriptNode : public BaseNode {
public:
    SubscriptNode(std::unique_ptr<BaseNode> array, 
            std::unique_ptr<BaseNode> subscript);

    bool is_lvalue() const override;
    void resolve_locals(Serializer &serializer, ScopeTracker &scopes) override;
    void serialize(Serializer &serializer) const override;
    void serialize_load_address(Serializer &serializer) const override;

    void print(TreePrinter &printer) const override;
private:
    std::unique_ptr<BaseNode> m_array;
    std::unique_ptr<BaseNode> m_subscript;
};

class BlockNode : public BaseNode {
public:
    BlockNode(std::vector<std::unique_ptr<BaseNode>> children);

    void resolve_globals(
            Serializer &serializer, SymbolMap &symbol_map) override;
    void resolve_locals(Serializer &serializer, ScopeTracker &scopes) override;
    void serialize(Serializer &serializer) const override;

    void print(TreePrinter &printer) const override;
private:
    std::vector<std::unique_ptr<BaseNode>> m_statements;
    SymbolMap m_scope_map;
};

// Block which introduces a new scope: statement blocks
class ScopedBlockNode : public BaseNode {
public:
    ScopedBlockNode(std::vector<std::unique_ptr<BaseNode>> children);

    void resolve_locals(Serializer &serializer, ScopeTracker &scopes) override;
    void serialize(Serializer &serializer) const override;

    void print(TreePrinter &printer) const override;
private:
    std::vector<std::unique_ptr<BaseNode>> m_statements;
    SymbolMap m_scope_map;
};

class CallableNode : public BaseNode {
public:
    CallableNode(Token token, Token ident, 
            CallableSignature signature, std::unique_ptr<BaseNode> body);
    
    bool is_matching_call(
            std::unique_ptr<ExpressionListNode> const &params) const;
    virtual void serialize_call(Serializer &serializer, 
            std::unique_ptr<ExpressionListNode> const &params) const = 0;

    Token const &ident() const;
    std::vector<Token> const &params() const;
    uint32_t n_params() const;
protected:
    std::unique_ptr<BaseNode> m_body;
private:
    Token m_ident;
    CallableSignature m_signature;
};

class FunctionNode : public CallableNode {
public:
    FunctionNode(Token token, Token ident, 
            CallableSignature signature, std::unique_ptr<BaseNode> body);

    void resolve_globals(
            Serializer &serializer, SymbolMap &symbol_map) override;
    void resolve_locals(Serializer &serializer, ScopeTracker &scopes) override;
    void serialize(Serializer &serializer) const override;
    void serialize_call(Serializer &serializer, 
            std::unique_ptr<ExpressionListNode> const &params) const override;

    void print(TreePrinter &printer) const override;

    std::string label() const;
private:
    uint32_t m_frame_size;
};

class InlineNode : public CallableNode {
public:
    InlineNode(Token token, Token ident, 
            CallableSignature signature, std::unique_ptr<BaseNode> body);

    void resolve_globals(
            Serializer &serializer, SymbolMap &symbol_map) override;
    void resolve_locals(Serializer &serializer, ScopeTracker &scopes) override;
    void serialize(Serializer &serializer) const override;
    void serialize_call(Serializer &serializer, 
            std::unique_ptr<ExpressionListNode> const &params) const override;

    void print(TreePrinter &printer) const override;

    std::string get_label() const;
private:
    std::vector<SymbolId> m_param_ids;
};

class LambdaNode : public CallableNode {
public:
    LambdaNode(Token token, 
            CallableSignature signature, std::unique_ptr<BaseNode> body);

    void resolve_locals(Serializer &serializer, ScopeTracker &scopes) override;
    void serialize(Serializer &serializer) const override;
    // Note: lambda call is serialized upon call to serialize,
    // body is also added as new code job
    void serialize_call(Serializer &serializer, 
            std::unique_ptr<ExpressionListNode> const &params) const override;

    void print(TreePrinter &printer) const override;

    std::string label() const;
};

class TypeDeclarationNode : public BaseNode {
public:
    TypeDeclarationNode(Token token, Token ident);

    void resolve_globals(
            Serializer &serializer, SymbolMap &symbol_map) override;
    void serialize(Serializer &serializer) const override;

    void print(TreePrinter &printer) const override;

    std::string label() const override;
private:
    Token m_ident;
};

class ExpressionListNode : public BaseNode {
public:
    ExpressionListNode(Token token, 
            std::vector<std::unique_ptr<BaseNode>> children);

    void resolve_locals(Serializer &serializer, ScopeTracker &scopes) override;
    void serialize(Serializer &serializer) const override;

    void print(TreePrinter &printer) const override;

    std::vector<std::unique_ptr<BaseNode>> const &exprs();
private:
    std::vector<std::unique_ptr<BaseNode>> m_exprs;
};

class IfNode : public BaseNode {
public:
    IfNode(Token token, std::unique_ptr<BaseNode> cond, 
            std::unique_ptr<BaseNode> case_true);

    void resolve_locals(Serializer &serializer, ScopeTracker &scopes) override;
    void serialize(Serializer &serializer) const override;

    void print(TreePrinter &printer) const override;
private:
    std::unique_ptr<BaseNode> m_cond;
    std::unique_ptr<BaseNode> m_case_true;
};

class ForLoopNode : public BaseNode {
public:
    ForLoopNode(Token token, 
            std::unique_ptr<BaseNode> init, std::unique_ptr<BaseNode> cond, 
            std::unique_ptr<BaseNode> post, std::unique_ptr<BaseNode> body);

    void resolve_locals(Serializer &serializer, ScopeTracker &scopes) override;
    void serialize(Serializer &serializer) const override;

    void print(TreePrinter &printer) const override;
private:
    std::unique_ptr<BaseNode> m_init;
    std::unique_ptr<BaseNode> m_cond;
    std::unique_ptr<BaseNode> m_post;
    std::unique_ptr<BaseNode> m_body;
};

class ReturnNode : public BaseNode {
public:
    ReturnNode(Token token, std::unique_ptr<BaseNode> operand);

    void resolve_locals(Serializer &serializer, ScopeTracker &scopes) override;
    void serialize(Serializer &serializer) const override;

    void print(TreePrinter &printer) const override;
private:
    std::unique_ptr<BaseNode> m_operand;
};

class VarDeclarationNode : public BaseNode {
public:
    VarDeclarationNode(Token token, Token ident, 
            std::unique_ptr<BaseNode> size, 
            std::unique_ptr<BaseNode> init_value);

    void resolve_globals(
            Serializer &serializer, SymbolMap &current);
    void resolve_locals(Serializer &serializer, ScopeTracker &scopes) override;
    void serialize(Serializer &serializer) const override;

    void print(TreePrinter &printer) const override;

    std::string label() const override;
private:
    uint32_t declared_size() const;

    Token m_ident;
    std::unique_ptr<BaseNode> m_size;
    std::unique_ptr<BaseNode> m_init_value;
};

class ExpressionStatementNode : public BaseNode {
public:
    ExpressionStatementNode(std::unique_ptr<BaseNode> child);
    
    void resolve_locals(Serializer &serializer, ScopeTracker &scopes) override;
    void serialize(Serializer &serializer) const override;

    void print(TreePrinter &printer) const override;
private:
    std::unique_ptr<BaseNode> m_expr;
};

#endif
