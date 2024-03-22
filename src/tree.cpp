#include "tree.hpp"
#include "utils.hpp"
#include <iostream>

BaseNode::BaseNode(Token token)
        : m_token(token), m_id(0) {}

BaseNode::~BaseNode() {}

bool BaseNode::is_lvalue() const {
    return false;
}

void BaseNode::resolve_globals(Serializer &, SymbolMap &) {}

void BaseNode::resolve_locals(Serializer &, ScopeTracker &) {}

void BaseNode::serialize_load_address(Serializer &) const {}

std::optional<uint32_t> BaseNode::get_constant_value() const {
    return std::nullopt;
}

std::string BaseNode::label() const {
    return m_token.data();
}

Token BaseNode::token() const {
    return m_token;
}

void BaseNode::set_id(SymbolId id) {
    m_id = id;
}

SymbolId BaseNode::id() const {
    return m_id;
}

TypeNode::TypeNode(Token token)
        : BaseNode(token) {}

void TypeNode::serialize(Serializer &) const {}

NamedTypeNode::NamedTypeNode(Token ident)
        : TypeNode(ident) {}

TypeListNode::TypeListNode(std::vector<std::unique_ptr<TypeNode>> type_list)
        : TypeNode(Token::synthetic("<type-list>")), 
        m_type_list(std::move(type_list)) {}

CallableTypeNode::CallableTypeNode(Token token, 
        std::unique_ptr<TypeListNode> param_types, 
        std::unique_ptr<TypeNode> return_type)
        : TypeNode(token), m_param_types(std::move(param_types)), 
        m_return_type(std::move(return_type)) {}

CallableSignature::CallableSignature(std::vector<Token> params, 
        std::unique_ptr<CallableTypeNode> type)
        : params(params), type(std::move(type)) {}

EmptyNode::EmptyNode()
        : BaseNode(Token::synthetic("<empty>")) {}

void EmptyNode::serialize(Serializer &) const {}

IntLitNode::IntLitNode(Token token)
        : BaseNode(token), m_value(token.to_int()) {
}

void IntLitNode::serialize(Serializer &serializer) const {
    serializer.add_instr(OpCode::Push, m_value);
}

std::optional<uint32_t> IntLitNode::get_constant_value() const {
    return m_value;
}

VariableNode::VariableNode(Token token)
        : BaseNode(token) {}

bool VariableNode::is_lvalue() const {
    return true;
}

void VariableNode::resolve_locals(Serializer &, ScopeTracker &scopes) {
    SymbolId id = lookup_symbol(token().data(), scopes);
    set_id(id);
}

void VariableNode::serialize(Serializer &serializer) const {
    SymbolEntry entry = serializer.symbol_table().get(id());
    switch (entry.storage_type) {
        case StorageType::AbsoluteRef:
            serializer.add_instr(OpCode::Push, entry.id, true);
            break;
        case StorageType::RelativeRef:
            serializer.add_instr(OpCode::LoadAddrRel, entry.value);
            break;
        case StorageType::Absolute:
            serializer.add_instr(OpCode::LoadAbs, entry.id, true);
            break;
        case StorageType::Relative:
            serializer.add_instr(OpCode::LoadRel, entry.value);
            break;
        case StorageType::Callable:
            serializer.push_callable_addr(entry.id);
            break;
        case StorageType::InlineReference:
            serializer.inline_frames().get(entry.id)->serialize(serializer);
            break;
        default:
            throw std::runtime_error("Invalid storage type");
    }
}

void VariableNode::serialize_load_address(Serializer &serializer) const {
    SymbolEntry entry = serializer.symbol_table().get(id());
    switch (entry.storage_type) {
        case StorageType::AbsoluteRef:
        case StorageType::RelativeRef:
            std::cerr << entry.symbol << std::endl;
            throw std::runtime_error("Cannot load address of reference");
        case StorageType::Absolute:
            serializer.add_instr(OpCode::Push, entry.id, true);
            break;
        case StorageType::Relative:
            serializer.add_instr(OpCode::LoadAddrRel, entry.value);
            break;
        case StorageType::InlineReference:
        serializer.inline_frames().get(entry.id)
                ->serialize_load_address(serializer);
            break;
        default:
            throw std::runtime_error("Invalid storage type");
    }
}

AssignNode::AssignNode(Token token, std::unique_ptr<BaseNode> left, 
        std::unique_ptr<BaseNode> right)
        : BaseNode(token), m_target(std::move(left)), m_expr(std::move(right)) {
    if (!m_target->is_lvalue()) {
        throw std::runtime_error("Expected lvalue as assignment target");
    }
}

void AssignNode::resolve_locals(Serializer &serializer, ScopeTracker &scopes) {
    m_target->resolve_locals(serializer, scopes);
    m_expr->resolve_locals(serializer, scopes);
}

void AssignNode::serialize(Serializer &serializer) const {
    m_target->serialize_load_address(serializer);
    m_expr->serialize(serializer);
    serializer.add_instr(OpCode::Binary, FuncCode::Assign);
}

AndNode::AndNode(Token token, std::unique_ptr<BaseNode> left, 
        std::unique_ptr<BaseNode> right)
        : BaseNode(token), m_left(std::move(left)), 
        m_right(std::move(right)) {}

void AndNode::resolve_locals(Serializer &serializer, ScopeTracker &scopes) {
    m_left->resolve_locals(serializer, scopes);
    m_right->resolve_locals(serializer, scopes);
}

void AndNode::serialize(Serializer &serializer) const {
    Label label_false = serializer.get_label();
    Label label_end = serializer.get_label();

    m_left->serialize(serializer);
    serializer.add_instr(OpCode::BrFalse, label_false, true);

    m_right->serialize(serializer);
    serializer.add_instr(OpCode::BrFalse, label_false, true);
    serializer.add_instr(OpCode::Push, 1);
    serializer.add_instr(OpCode::Jump, label_end, true);

    serializer.add_label(label_false);
    serializer.add_instr(OpCode::Push, 0);

    serializer.add_label(label_end);
}

OrNode::OrNode(Token token, std::unique_ptr<BaseNode> left, 
        std::unique_ptr<BaseNode> right)
        : BaseNode(token), m_left(std::move(left)), 
        m_right(std::move(right)) {}

void OrNode::resolve_locals(Serializer &serializer, ScopeTracker &scopes) {
    m_left->resolve_locals(serializer, scopes);
    m_right->resolve_locals(serializer, scopes);
}

void OrNode::serialize(Serializer &serializer) const {
    Label label_true = serializer.get_label();
    Label label_end = serializer.get_label();

    m_left->serialize(serializer);
    serializer.add_instr(OpCode::BrTrue, label_true, true);

    m_right->serialize(serializer);
    serializer.add_instr(OpCode::BrTrue, label_true, true);
    serializer.add_instr(OpCode::Push, 0);
    serializer.add_instr(OpCode::Jump, label_end, true);

    serializer.add_label(label_true);
    serializer.add_instr(OpCode::Push, 1);

    serializer.add_label(label_end);
}

AddressOfNode::AddressOfNode(Token token, std::unique_ptr<BaseNode> operand)
        : BaseNode(token), m_operand(std::move(operand)) {}

void AddressOfNode::resolve_locals(Serializer &serializer, 
        ScopeTracker &scopes) {
    m_operand->resolve_locals(serializer, scopes);
}

void AddressOfNode::serialize(Serializer &serializer) const {
    m_operand->serialize_load_address(serializer);
}

DereferenceNode::DereferenceNode(Token token, 
        std::unique_ptr<BaseNode> operand)
        : BaseNode(token), m_operand(std::move(operand)) {}

bool DereferenceNode::is_lvalue() const {
    return true;
}

void DereferenceNode::resolve_locals(Serializer &serializer, 
        ScopeTracker &scopes) {
    m_operand->resolve_locals(serializer, scopes);
}

void DereferenceNode::serialize(Serializer &serializer) const {
    m_operand->serialize(serializer);
    serializer.add_instr(OpCode::LoadAbs);
}

void DereferenceNode::serialize_load_address(Serializer &serializer) const {
    m_operand->serialize(serializer);
}

IfElseNode::IfElseNode(Token token, std::unique_ptr<BaseNode> cond, 
        std::unique_ptr<BaseNode> case_true, 
        std::unique_ptr<BaseNode> case_false)
        : BaseNode(token), m_cond(std::move(cond)), 
        m_case_true(std::move(case_true)), 
        m_case_false(std::move(case_false)) {}

void IfElseNode::resolve_locals(Serializer &serializer, 
        ScopeTracker &scopes) {
    m_cond->resolve_locals(serializer, scopes);
    m_case_true->resolve_locals(serializer, scopes);
    m_case_false->resolve_locals(serializer, scopes);
}

void IfElseNode::serialize(Serializer &serializer) const {
    Label label_false = serializer.get_label();
    Label label_end = serializer.get_label();
    
    m_cond->serialize(serializer);
    serializer.add_instr(OpCode::BrFalse, label_false, true);

    m_case_true->serialize(serializer);
    serializer.add_instr(OpCode::Jump, label_end, true);

    serializer.add_label(label_false);
    m_case_false->serialize(serializer);

    serializer.add_label(label_end);
}

CallNode::CallNode(std::unique_ptr<BaseNode> func, 
        std::unique_ptr<ExpressionListNode> args)
        : BaseNode(Token::synthetic("<call>")), m_func(std::move(func)), 
        m_args(std::move(args)) {}

std::unique_ptr<BaseNode> CallNode::make_call(Token ident, 
        std::vector<std::unique_ptr<BaseNode>> params) {
    return std::make_unique<CallNode>(
            std::make_unique<VariableNode>(ident), 
            std::make_unique<ExpressionListNode>(
                Token::synthetic("<params>"), std::move(params)));
}

std::unique_ptr<BaseNode> CallNode::make_unary_call(Token ident, 
        std::unique_ptr<BaseNode> param) {
    std::vector<std::unique_ptr<BaseNode>> params;
    params.push_back(std::move(param));
    return CallNode::make_call(ident, std::move(params));
}

std::unique_ptr<BaseNode> CallNode::make_binary_call(Token ident, 
        std::unique_ptr<BaseNode> left, 
        std::unique_ptr<BaseNode> right) {
    std::vector<std::unique_ptr<BaseNode>> params;
    params.push_back(std::move(left));
    params.push_back(std::move(right));
    return CallNode::make_call(ident, std::move(params));
}

void CallNode::resolve_locals(Serializer &serializer, 
        ScopeTracker &scopes) {
    m_func->resolve_locals(serializer, scopes);
    m_args->resolve_locals(serializer, scopes);
}

void CallNode::serialize(Serializer &serializer) const {
    SymbolId id = m_func->id();
    SymbolEntry entry = serializer.symbol_table().get(id);
    if (entry.storage_type == StorageType::Intrinsic) {
        IntrinsicEntry intrinsic = intrinsics[entry.value];
        if (m_args->exprs().size() != intrinsic.n_args) {
            throw std::runtime_error(
                    "Invalid intrinsic invocation of " + intrinsic.symbol);
        }
        m_args->serialize(serializer);
        serializer.add_instr(intrinsic.opcode, intrinsic.funccode);
    } else if (entry.storage_type == StorageType::Callable) {
        serializer.call(id, m_args);
    } else {
        m_args->serialize(serializer);
        serializer.add_instr(OpCode::Push, m_args->exprs().size());
        m_func->serialize(serializer);
        serializer.add_instr(OpCode::Call);
    }
}

SubscriptNode::SubscriptNode(std::unique_ptr<BaseNode> array, 
        std::unique_ptr<BaseNode> subscript)
        : BaseNode(Token::synthetic("<subscript>")), m_array(std::move(array)),
        m_subscript(std::move(subscript)) {}

bool SubscriptNode::is_lvalue() const {
    return true;
}

void SubscriptNode::resolve_locals(Serializer &serializer, 
        ScopeTracker &scopes) {
    m_array->resolve_locals(serializer, scopes);
    m_subscript->resolve_locals(serializer, scopes);
}

void SubscriptNode::serialize(Serializer &serializer) const {
    serialize_load_address(serializer);
    serializer.add_instr(OpCode::LoadAbs);
}

void SubscriptNode::serialize_load_address(Serializer &serializer) const {
    m_array->serialize(serializer);
    m_subscript->serialize(serializer);
    serializer.add_instr(OpCode::Binary, FuncCode::Add);
}

BlockNode::BlockNode(std::vector<std::unique_ptr<BaseNode>> statements)
        : BaseNode(Token::synthetic("<block>")), 
        m_statements(std::move(statements)), m_scope_map() {}

void BlockNode::resolve_globals(
        Serializer &serializer, SymbolMap &symbol_map) {
    for (std::unique_ptr<BaseNode> const &stmt : m_statements) {
        stmt->resolve_globals(serializer, symbol_map);
    }
}

void BlockNode::resolve_locals(Serializer &serializer, 
        ScopeTracker &scopes) {
    for (std::unique_ptr<BaseNode> const &stmt : m_statements) {
        stmt->resolve_locals(serializer, scopes);
    }
}

void BlockNode::serialize(Serializer &serializer) const {
    for (std::unique_ptr<BaseNode> const &stmt : m_statements) {
        stmt->serialize(serializer);
    }
}

ScopedBlockNode::ScopedBlockNode(
        std::vector<std::unique_ptr<BaseNode>> statements)
        : BaseNode(Token::synthetic("<scoped-block>")), 
        m_statements(std::move(statements)), m_scope_map() {}

void ScopedBlockNode::resolve_locals(Serializer &serializer, 
        ScopeTracker &scopes) {
    ScopeTracker block_scopes(scopes.global, scopes.enclosing, {});
    for (auto const &entry : scopes.current) {
        block_scopes.enclosing[entry.first] = entry.second;
    }
    for (std::unique_ptr<BaseNode> const &stmt : m_statements) {
        stmt->resolve_locals(serializer, block_scopes);
    }
}

void ScopedBlockNode::serialize(Serializer &serializer) const {
    for (std::unique_ptr<BaseNode> const &stmt : m_statements) {
        stmt->serialize(serializer);
    }
}

CallableNode::CallableNode(Token token, Token ident, 
        CallableSignature signature, std::unique_ptr<BaseNode> body)
        : BaseNode(token), m_body(std::move(body)), m_ident(ident),
        m_signature(std::move(signature)) {}

bool CallableNode::is_matching_call(
        std::unique_ptr<ExpressionListNode> const &params) const {
    return params->exprs().size() == n_params();
}

Token const &CallableNode::ident() const {
    return m_ident;
}
std::vector<Token> const &CallableNode::params() const {
    return m_signature.params;
}

uint32_t CallableNode::n_params() const {
    return m_signature.params.size();
}

FunctionNode::FunctionNode(Token token, Token ident, 
        CallableSignature signature, std::unique_ptr<BaseNode> body)
        : CallableNode(token, ident, std::move(signature), std::move(body)) {}

void FunctionNode::resolve_globals(
        Serializer &serializer, SymbolMap &symbol_map) {
    set_id(serializer.declare_callable(ident().data(), 
            symbol_map, this));
    serializer.add_job(id(), this, false);
}

void FunctionNode::resolve_locals(Serializer &serializer, 
        ScopeTracker &scopes) {
    // todo replace by block: -> {{ fn (...) {...} }}
    ScopeTracker block_scopes(scopes.global, scopes.enclosing, {}); 
    uint32_t position = -3 - n_params();
    for (Token const &param : params()) {
        serializer.symbol_table().declare(param.data(), block_scopes.current, 
                StorageType::Relative, position);
        position++;
    }
    serializer.symbol_table().open_container();
    m_body->resolve_locals(serializer, block_scopes);
    m_frame_size = serializer.symbol_table().container_size();
    serializer.symbol_table().resolve_local_container();
}

void FunctionNode::serialize(Serializer &serializer) const {
    if (id() == 0) {
        throw std::runtime_error("Unresolved name");
    }
    serializer.add_instr(OpCode::AddSp, m_frame_size);
    m_body->serialize(serializer);
    serializer.add_instr(OpCode::Ret, 0);
}

void FunctionNode::serialize_call(Serializer &serializer, 
        std::unique_ptr<ExpressionListNode> const &params) const {
    if (params != nullptr) {
        params->serialize(serializer);
    }
    serializer.add_instr(OpCode::Push, n_params());
    serializer.add_instr(OpCode::Push, id(), true);
    serializer.add_instr(OpCode::Call);
}

std::string FunctionNode::label() const {
    return token().data() + " " + ident().data() + "(" 
            + tokenlist_to_string(params()) + ")";
}

InlineNode::InlineNode(Token token, Token ident, 
        CallableSignature signature, std::unique_ptr<BaseNode> body)
        : CallableNode(token, ident, std::move(signature), std::move(body)), 
        m_param_ids() {}

void InlineNode::resolve_globals(
        Serializer &serializer, SymbolMap &symbol_map) {
    set_id(serializer.declare_callable(ident().data(), 
            symbol_map, this));
    serializer.add_job(id(), this, true);
}

void InlineNode::resolve_locals(Serializer &serializer, ScopeTracker &scopes) {
    ScopeTracker block_scopes(scopes.global, scopes.enclosing, {});
    uint32_t position = 0;
    for (Token const &param : params()) {
        SymbolId id = serializer.symbol_table().declare(param.data(), 
                block_scopes.current, StorageType::InlineReference, position);
        m_param_ids.push_back(id);
        position++;
    }
    m_body->resolve_locals(serializer, block_scopes);
}

void InlineNode::serialize(Serializer &) const {}

void InlineNode::serialize_call(Serializer &serializer, 
        std::unique_ptr<ExpressionListNode> const &params) const {
    serializer.inline_frames().open_call(params, m_param_ids);
    m_body->serialize(serializer);
    serializer.inline_frames().close_call(m_param_ids);
}

std::string InlineNode::get_label() const {
    return token().data() + " " + ident().data() + "(" 
            + tokenlist_to_string(params()) + ")";
}

LambdaNode::LambdaNode(Token token, CallableSignature signature, 
        std::unique_ptr<BaseNode> expr)
        : CallableNode(
            token, Token::synthetic("<anonymous>"), 
            std::move(signature), std::move(expr)) {}

void LambdaNode::resolve_locals(Serializer &serializer, ScopeTracker &scopes) {
    ScopeTracker block_scopes(scopes.global, scopes.enclosing, {});
    uint32_t position = -3 - n_params();
    for (Token const &param : params()) {
        serializer.symbol_table().declare(param.data(), block_scopes.current, 
                StorageType::Relative, position);
        position++;
    }
    m_body->resolve_locals(serializer, block_scopes);
}

void LambdaNode::serialize(Serializer &serializer) const {
    SymbolId id = serializer.get_label();
    serializer.add_job(id, m_body.get(), false);
    serializer.add_instr(OpCode::Push, id, true);
}

void LambdaNode::serialize_call(Serializer &, 
        std::unique_ptr<ExpressionListNode> const &) const {}

std::string LambdaNode::label() const {
    return token().data() + " (" + tokenlist_to_string(params()) + ")";
}

TypeDeclarationNode::TypeDeclarationNode(
        Token token, Token ident)
        : BaseNode(token), m_ident(ident) {}

void TypeDeclarationNode::resolve_globals(
        Serializer &serializer, SymbolMap &symbol_map) {
    set_id(serializer.symbol_table().declare(
            m_ident.data(), symbol_map, StorageType::Type));
}

void TypeDeclarationNode::serialize(Serializer &) const {}

std::string TypeDeclarationNode::label() const {
    return token().data() + " " + m_ident.data();
}

ExpressionListNode::ExpressionListNode(
        Token token, std::vector<std::unique_ptr<BaseNode>> exprs)
        : BaseNode(token), m_exprs(std::move(exprs)) {}

void ExpressionListNode::resolve_locals(Serializer &serializer, 
        ScopeTracker &scopes) {
    for (std::unique_ptr<BaseNode> const &expr : m_exprs) {
        expr->resolve_locals(serializer, scopes);
    }
}

void ExpressionListNode::serialize(Serializer &serializer) const {
    for (std::unique_ptr<BaseNode> const &expr : m_exprs) {
        expr->serialize(serializer);
    }
}

std::vector<std::unique_ptr<BaseNode>> const &ExpressionListNode::exprs() {
    return m_exprs;
}

IfNode::IfNode(Token token, std::unique_ptr<BaseNode> cond, 
        std::unique_ptr<BaseNode> case_true)
        : BaseNode(token), m_cond(std::move(cond)), 
        m_case_true(std::move(case_true)) {}

void IfNode::resolve_locals(Serializer &serializer, 
        ScopeTracker &scopes) {
    m_cond->resolve_locals(serializer, scopes);
    m_case_true->resolve_locals(serializer, scopes);
}

void IfNode::serialize(Serializer &serializer) const {
    Label label_end = serializer.get_label();
    m_cond->serialize(serializer);
    serializer.add_instr(OpCode::BrFalse, label_end, true);
    m_case_true->serialize(serializer);
    serializer.add_label(label_end);
}

ForLoopNode::ForLoopNode(Token token, 
        std::unique_ptr<BaseNode> init, std::unique_ptr<BaseNode> cond, 
        std::unique_ptr<BaseNode> post, std::unique_ptr<BaseNode> body)
        : BaseNode(token), m_init(std::move(init)), m_cond(std::move(cond)), 
        m_post(std::move(post)), m_body(std::move(body)) {}

void ForLoopNode::resolve_locals(Serializer &serializer, 
        ScopeTracker &scopes) {
    m_init->resolve_locals(serializer, scopes);
    m_cond->resolve_locals(serializer, scopes);
    m_post->resolve_locals(serializer, scopes);
    m_body->resolve_locals(serializer, scopes);
}

void ForLoopNode::serialize(Serializer &serializer) const {
    Label loop_body_label = serializer.get_label();
    Label cond_label = serializer.get_label();

    m_init->serialize(serializer); // init: statement
    serializer.add_instr(OpCode::Jump, cond_label, true);
    
    serializer.add_label(loop_body_label); // body and post: statements
    m_body->serialize(serializer);
    m_post->serialize(serializer);

    serializer.add_label(cond_label);
    m_cond->serialize(serializer); // cond: expression
    serializer.add_instr(OpCode::BrTrue, loop_body_label, true);
}

ReturnNode::ReturnNode(Token token, std::unique_ptr<BaseNode> operand)
        : BaseNode(token), m_operand(std::move(operand)) {}

void ReturnNode::resolve_locals(Serializer &serializer, 
        ScopeTracker &scopes) {
    m_operand->resolve_locals(serializer, scopes);
}

void ReturnNode::serialize(Serializer &serializer) const {
    m_operand->serialize(serializer);
    serializer.add_instr(OpCode::Ret);
}

VarDeclarationNode::VarDeclarationNode(Token token, Token ident, 
        std::unique_ptr<BaseNode> size, 
        std::unique_ptr<BaseNode> init_value)
        : BaseNode(token), m_ident(ident), m_size(std::move(size)), 
        m_init_value(std::move(init_value)) {}

void VarDeclarationNode::resolve_globals(
        Serializer &serializer, SymbolMap &current) {
    if (m_init_value != nullptr) {
        throw std::runtime_error("not implemented");
    }
    set_id(serializer.symbol_table().declare(
            m_ident.data(), current, 
            m_size == nullptr ? 
                StorageType::Absolute : StorageType::AbsoluteRef, 
            0, declared_size()));
    serializer.symbol_table().add_to_container(id());
}

void VarDeclarationNode::resolve_locals(Serializer &serializer, 
        ScopeTracker &scopes) {
    if (m_init_value != nullptr) {
        m_init_value->resolve_locals(serializer, scopes);
    }
    set_id(serializer.symbol_table().declare(
            m_ident.data(), scopes.current, 
            m_size == nullptr ? 
                StorageType::Relative : StorageType::RelativeRef, 
            0, declared_size()));
    serializer.symbol_table().add_to_container(id());
}

void VarDeclarationNode::serialize(Serializer &serializer) const {
    if (m_init_value != nullptr) {
        SymbolEntry entry = serializer.symbol_table().get(id());
        serializer.add_instr(OpCode::LoadAddrRel, entry.value);
        m_init_value->serialize(serializer);
        serializer.add_instr(OpCode::Binary, FuncCode::Assign);
        serializer.add_instr(OpCode::Pop);
    }
}

std::string VarDeclarationNode::label() const {
    return token().data() + " " + m_ident.data();
}

uint32_t VarDeclarationNode::declared_size() const {
    if (m_size == nullptr) {
        return 1;
    }
    std::optional<uint32_t> const_value = m_size->get_constant_value();
    if (const_value.has_value()) {
        return const_value.value();
    }
    throw std::runtime_error("Expected constant value as size");
}

ExpressionStatementNode::ExpressionStatementNode(std::unique_ptr<BaseNode> expr)
        : BaseNode(Token::synthetic("<expr-stmt>")), m_expr(std::move(expr)) {}

void ExpressionStatementNode::resolve_locals(Serializer &serializer, 
        ScopeTracker &scopes) {
    m_expr->resolve_locals(serializer, scopes);
}

void ExpressionStatementNode::serialize(Serializer &serializer) const {
    m_expr->serialize(serializer);
    serializer.add_instr(OpCode::Pop);
}
