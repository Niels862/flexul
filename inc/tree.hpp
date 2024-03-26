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

class TypeNode : public BaseNode {
public:
    TypeNode(Token token);

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

class ExpressionNode : public BaseNode {
public:
    ExpressionNode(Token token);
};

class StatementNode : public BaseNode {
public:
    StatementNode(Token token);
};

class VariableNode : public ExpressionNode {
public:
    VariableNode(Token token);

    bool is_lvalue() const override;
    void resolve_locals(Serializer &serializer, ScopeTracker &scopes) override;
    void serialize(Serializer &serializer) const override;
    void serialize_load_address(Serializer &serializer) const override;

    void print(TreePrinter &printer) const override;
};

class LiteralNode : public ExpressionNode {
public:
    LiteralNode(Token token);

    void resolve_locals(Serializer &serializer, 
            ScopeTracker &scopes) override;

    void print(TreePrinter &printer) const override;
};

class IntegerLiteralNode : public LiteralNode {
public:
    IntegerLiteralNode(Token token);

    void serialize(Serializer &serializer) const override;
    std::optional<uint32_t> get_constant_value() const;
private:
    uint32_t m_value;
};

class UnaryExpressionNode : public ExpressionNode {
public:
    UnaryExpressionNode(Token token, std::unique_ptr<ExpressionNode> operand);

    void resolve_locals(Serializer &serializer, ScopeTracker &scopes) override;

    void print(TreePrinter &printer) const override;
protected:
    // todo BaseNode -> ExpressionNode
    std::unique_ptr<ExpressionNode> m_operand;
};

class AddressOfNode : public UnaryExpressionNode {
public:
    AddressOfNode(Token token, std::unique_ptr<ExpressionNode> operand);

    void serialize(Serializer &serializer) const override;
};

class DereferenceNode : public UnaryExpressionNode {
public:
    DereferenceNode(Token token, std::unique_ptr<ExpressionNode> operand);

    bool is_lvalue() const override;
    void serialize(Serializer &serializer) const override;
    void serialize_load_address(Serializer &serializer) const override;
};

class BinaryExpressionNode : public ExpressionNode {
public:
    BinaryExpressionNode(Token token, std::unique_ptr<ExpressionNode> left, 
            std::unique_ptr<ExpressionNode> right);

    void resolve_locals(Serializer &serializer, ScopeTracker &scopes) override;

    void print(TreePrinter &printer) const override;
protected:
    std::unique_ptr<ExpressionNode> m_left;
    std::unique_ptr<ExpressionNode> m_right;
};

class AssignNode : public BinaryExpressionNode {
public:
    AssignNode(Token token, std::unique_ptr<ExpressionNode> left, 
            std::unique_ptr<ExpressionNode> right);

    void serialize(Serializer &serializer) const override;
};

class AndNode : public BinaryExpressionNode {
public:
    AndNode(Token token, std::unique_ptr<ExpressionNode> left, 
            std::unique_ptr<ExpressionNode> right);

    void serialize(Serializer &serializer) const override;
};

class OrNode : public BinaryExpressionNode {
public:
    OrNode(Token token, std::unique_ptr<ExpressionNode> left, 
            std::unique_ptr<ExpressionNode> right);

    void serialize(Serializer &serializer) const override;
};

class SubscriptNode : public BinaryExpressionNode {
public:
    SubscriptNode(std::unique_ptr<ExpressionNode> array, 
            std::unique_ptr<ExpressionNode> subscript);

    bool is_lvalue() const override;
    void serialize(Serializer &serializer) const override;
    void serialize_load_address(Serializer &serializer) const override;
};

class CallNode : public ExpressionNode {
public:
    CallNode(std::unique_ptr<ExpressionNode> func, 
            std::unique_ptr<ExpressionListNode> args);

    static std::unique_ptr<CallNode> make_call(Token ident, 
            std::vector<std::unique_ptr<ExpressionNode>> params);
    static std::unique_ptr<CallNode> make_unary_call(Token ident, 
            std::unique_ptr<ExpressionNode> param);
    static std::unique_ptr<CallNode> make_binary_call(Token ident, 
            std::unique_ptr<ExpressionNode> left, 
            std::unique_ptr<ExpressionNode> right);

    void resolve_locals(Serializer &serializer, ScopeTracker &scopes) override;
    void serialize(Serializer &serializer) const override;

    void print(TreePrinter &printer) const override;
private:
    std::unique_ptr<ExpressionNode> m_func;
    std::unique_ptr<ExpressionListNode> m_args;
};

class TernaryNode : public ExpressionNode {
public:
    TernaryNode(Token token, std::unique_ptr<ExpressionNode> cond, 
            std::unique_ptr<ExpressionNode> case_true, 
            std::unique_ptr<ExpressionNode> case_false);

    void resolve_locals(Serializer &serializer, ScopeTracker &scopes) override;
    void serialize(Serializer &serializer) const override;

    void print(TreePrinter &printer) const override;
private:
    std::unique_ptr<ExpressionNode> m_cond;
    std::unique_ptr<ExpressionNode> m_case_true;
    std::unique_ptr<ExpressionNode> m_case_false;
};

class LambdaNode : public ExpressionNode {
public:
    LambdaNode(Token token, 
            CallableSignature signature, 
            std::unique_ptr<StatementNode> body);

    void resolve_locals(Serializer &serializer, ScopeTracker &scopes) override;
    void serialize(Serializer &serializer) const override;

    void print(TreePrinter &printer) const override;

    std::string label() const;
private:
    std::unique_ptr<StatementNode> m_body;
    CallableSignature m_signature;
};

class CallableNode : public StatementNode {
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


class EmptyNode : public StatementNode {
public:
    EmptyNode();

    void serialize(Serializer &serializer) const override;

    void print(TreePrinter &printer) const override;
};

class BlockNode : public StatementNode {
public:
    BlockNode(std::vector<std::unique_ptr<StatementNode>> children);

    void resolve_globals(
            Serializer &serializer, SymbolMap &symbol_map) override;
    void resolve_locals(Serializer &serializer, ScopeTracker &scopes) override;
    void serialize(Serializer &serializer) const override;

    void print(TreePrinter &printer) const override;
private:
    std::vector<std::unique_ptr<StatementNode>> m_statements;
    SymbolMap m_scope_map;
};

// Block which introduces a new scope: statement blocks
class ScopedBlockNode : public StatementNode {
public:
    ScopedBlockNode(std::vector<std::unique_ptr<StatementNode>> children);

    void resolve_locals(Serializer &serializer, ScopeTracker &scopes) override;
    void serialize(Serializer &serializer) const override;

    void print(TreePrinter &printer) const override;
private:
    std::vector<std::unique_ptr<StatementNode>> m_statements;
    SymbolMap m_scope_map;
};

class TypeDeclarationNode : public StatementNode {
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
            std::vector<std::unique_ptr<ExpressionNode>> children);

    void resolve_locals(Serializer &serializer, ScopeTracker &scopes) override;
    void serialize(Serializer &serializer) const override;

    void print(TreePrinter &printer) const override;

    std::vector<std::unique_ptr<ExpressionNode>> const &exprs();
private:
    std::vector<std::unique_ptr<ExpressionNode>> m_exprs;
};

class IfNode : public StatementNode {
public:
    IfNode(Token token, std::unique_ptr<ExpressionNode> cond, 
            std::unique_ptr<StatementNode> case_true);

    void resolve_locals(Serializer &serializer, ScopeTracker &scopes) override;
    void serialize(Serializer &serializer) const override;

    void print(TreePrinter &printer) const override;
private:
    std::unique_ptr<ExpressionNode> m_cond;
    std::unique_ptr<StatementNode> m_case_true;
};

class IfElseNode : public StatementNode {
public:
    IfElseNode(Token token, std::unique_ptr<ExpressionNode> cond, 
            std::unique_ptr<StatementNode> case_true, 
            std::unique_ptr<StatementNode> case_false);

    void resolve_locals(Serializer &serializer, ScopeTracker &scopes) override;
    void serialize(Serializer &serializer) const override;

    void print(TreePrinter &printer) const override;
private:
    std::unique_ptr<ExpressionNode> m_cond;
    std::unique_ptr<StatementNode> m_case_true;
    std::unique_ptr<StatementNode> m_case_false;
};

class ForLoopNode : public StatementNode {
public:
    ForLoopNode(Token token, 
            std::unique_ptr<StatementNode> init, 
            std::unique_ptr<ExpressionNode> cond, 
            std::unique_ptr<StatementNode> post, 
            std::unique_ptr<StatementNode> body);

    void resolve_locals(Serializer &serializer, ScopeTracker &scopes) override;
    void serialize(Serializer &serializer) const override;

    void print(TreePrinter &printer) const override;
private:
    std::unique_ptr<StatementNode> m_init;
    std::unique_ptr<ExpressionNode> m_cond;
    std::unique_ptr<StatementNode> m_post;
    std::unique_ptr<StatementNode> m_body;
};

class ReturnNode : public StatementNode {
public:
    ReturnNode(Token token, std::unique_ptr<ExpressionNode> operand);

    void resolve_locals(Serializer &serializer, ScopeTracker &scopes) override;
    void serialize(Serializer &serializer) const override;

    void print(TreePrinter &printer) const override;
private:
    std::unique_ptr<ExpressionNode> m_operand;
};

class VarDeclarationNode : public StatementNode {
public:
    VarDeclarationNode(Token token, Token ident, 
            std::unique_ptr<ExpressionNode> size, 
            std::unique_ptr<ExpressionNode> init_value);

    void resolve_globals(
            Serializer &serializer, SymbolMap &current);
    void resolve_locals(Serializer &serializer, ScopeTracker &scopes) override;
    void serialize(Serializer &serializer) const override;

    void print(TreePrinter &printer) const override;

    std::string label() const override;
private:
    uint32_t declared_size() const;

    Token m_ident;
    std::unique_ptr<ExpressionNode> m_size;
    std::unique_ptr<ExpressionNode> m_init_value;
};

class ExpressionStatementNode : public StatementNode {
public:
    ExpressionStatementNode(std::unique_ptr<ExpressionNode> expr);
    
    void resolve_locals(Serializer &serializer, ScopeTracker &scopes) override;
    void serialize(Serializer &serializer) const override;

    void print(TreePrinter &printer) const override;
private:
    std::unique_ptr<ExpressionNode> m_expr;
};

#endif
