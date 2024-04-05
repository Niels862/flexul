#include "tree.hpp"
#include "treeprinter.hpp"
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

void TypeNode::resolve_globals(Serializer &, SymbolMap &) {}

void TypeNode::serialize(Serializer &) const {}

std::string to_string(TypeNode const *node) {
    if (node == nullptr) {
        return "Any";
    }
    return node->type_string();
}

NamedTypeNode::NamedTypeNode(Token ident)
        : TypeNode(ident) {}

void NamedTypeNode::resolve_locals(Serializer &, ScopeTracker &scopes) {
    set_id(lookup_symbol(token().data(), scopes));
}

void NamedTypeNode::print(TreePrinter &printer) const {
    printer.print_node(this);
}

std::string NamedTypeNode::type_string() const {
    return token().data();
}

TypeListNode::TypeListNode(std::vector<std::unique_ptr<TypeNode>> type_list)
        : TypeNode(Token::synthetic("<type-list>")), 
        m_type_list(std::move(type_list)) {}

void TypeListNode::resolve_locals(Serializer &serializer, 
        ScopeTracker &scopes) {
    for (auto const &entry : m_type_list) {
        entry->resolve_locals(serializer, scopes);
    }
}

void TypeListNode::print(TreePrinter &printer) const {
    printer.print_node(this);
    for (auto const &entry : m_type_list) {
        if (entry == m_type_list.back()) {
            printer.last_child(entry.get());
        } else {
            printer.next_child(entry.get());
        }
    }
}

std::vector<std::unique_ptr<TypeNode>> const &TypeListNode::list() const {
    return m_type_list;
}

std::string TypeListNode::type_string() const {
    std::string str = "(";
    for (auto const &entry : m_type_list) {
        str += to_string(entry.get());
        if (entry != m_type_list.back()) {
            str += ", ";
        }
    }
    return str + ")";
}

CallableTypeNode::CallableTypeNode(Token token, 
        std::unique_ptr<TypeListNode> param_types, 
        std::unique_ptr<TypeNode> return_type)
        : TypeNode(token), m_param_types(std::move(param_types)), 
        m_return_type(std::move(return_type)) {}

void CallableTypeNode::resolve_locals(Serializer &serializer, 
        ScopeTracker &scopes) {
    m_param_types->resolve_locals(serializer, scopes);
    m_return_type->resolve_locals(serializer, scopes);
}

void CallableTypeNode::print(TreePrinter &printer) const {
    printer.print_node(this);
    printer.next_child(m_param_types.get());
    printer.last_child(m_return_type.get());
}

std::string CallableTypeNode::type_string() const {
    return to_string(m_param_types.get()) 
            + " -> " + to_string(m_return_type.get());
}

TypeListNode *CallableTypeNode::param_types() const {
    return m_param_types.get();
}

TypeNode *CallableTypeNode::return_type() const {
    return m_return_type.get();
}

CallableSignature::CallableSignature(std::vector<Token> params, 
        std::unique_ptr<CallableTypeNode> type)
        : params(params), type(std::move(type)) {}


ExpressionNode::ExpressionNode(Token token, TypeNode *type)
        : BaseNode(token), m_type(type) {}

void ExpressionNode::resolve_globals(Serializer &, SymbolMap &) {}

TypeNode const *ExpressionNode::type() const {
    return m_type;
}

StatementNode::StatementNode(Token token)
        : BaseNode(token) {}

VariableNode::VariableNode(Token token)
        : ExpressionNode(token) {}

bool VariableNode::is_lvalue() const {
    return true;
}

void VariableNode::resolve_locals(Serializer &, ScopeTracker &scopes) {
    set_id(lookup_symbol(token().data(), scopes));
}

void VariableNode::serialize(Serializer &serializer) const {
    SymbolEntry const &entry = serializer.symbol_table().get(id());
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
    SymbolEntry const &entry = serializer.symbol_table().get(id());
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

void VariableNode::print(TreePrinter &printer) const {
    printer.print_node(this);
}

LiteralNode::LiteralNode(Token token, TypeNode *type)
        : ExpressionNode(token, type) {}

void LiteralNode::resolve_locals(Serializer &, ScopeTracker &) {}

void LiteralNode::print(TreePrinter &printer) const {
    printer.print_node(this);
}

IntegerLiteralNode::IntegerLiteralNode(Token token, TypeNode *type)
        : LiteralNode(token, type), m_value(token.to_int()) {
}

void IntegerLiteralNode::serialize(Serializer &serializer) const {
    serializer.add_instr(OpCode::Push, m_value);
}

std::optional<uint32_t> IntegerLiteralNode::get_constant_value() const {
    return m_value;
}

UnaryExpressionNode::UnaryExpressionNode(Token token, 
        std::unique_ptr<ExpressionNode> operand)
        : ExpressionNode(token), 
        m_operand(std::move(operand)) {}

void UnaryExpressionNode::print(TreePrinter &printer) const {
    printer.print_node(this);
    printer.last_child(m_operand.get());
}

void UnaryExpressionNode::resolve_locals(Serializer &serializer, 
        ScopeTracker &scopes) {
    m_operand->resolve_locals(serializer, scopes);
}

AddressOfNode::AddressOfNode(Token token, 
        std::unique_ptr<ExpressionNode> operand)
        : UnaryExpressionNode(token, std::move(operand)) {}

void AddressOfNode::serialize(Serializer &serializer) const {
    m_operand->serialize_load_address(serializer);
}

DereferenceNode::DereferenceNode(Token token, 
        std::unique_ptr<ExpressionNode> operand)
        : UnaryExpressionNode(token, std::move(operand)) {}

bool DereferenceNode::is_lvalue() const {
    return true;
}

void DereferenceNode::serialize(Serializer &serializer) const {
    m_operand->serialize(serializer);
    serializer.add_instr(OpCode::LoadAbs);
}

void DereferenceNode::serialize_load_address(Serializer &serializer) const {
    m_operand->serialize(serializer);
}

BinaryExpressionNode::BinaryExpressionNode(Token token, 
        std::unique_ptr<ExpressionNode> left, 
        std::unique_ptr<ExpressionNode> right)
        : ExpressionNode(token), 
        m_left(std::move(left)), m_right(std::move(right)) {}

void BinaryExpressionNode::resolve_locals(Serializer &serializer, 
        ScopeTracker &scopes) {
    m_left->resolve_locals(serializer, scopes);
    m_right->resolve_locals(serializer, scopes);
}

void BinaryExpressionNode::print(TreePrinter &printer) const {
    printer.print_node(this);
    printer.next_child(m_left.get());
    printer.last_child(m_right.get());
}

AssignNode::AssignNode(Token token, 
        std::unique_ptr<ExpressionNode> left, 
        std::unique_ptr<ExpressionNode> right)
        : BinaryExpressionNode(token, std::move(left), std::move(right)) {
    if (!m_left->is_lvalue()) {
        throw std::runtime_error("Expected lvalue as assignment target");
    }
}

void AssignNode::serialize(Serializer &serializer) const {
    m_left->serialize_load_address(serializer);
    m_right->serialize(serializer);
    serializer.add_instr(OpCode::Binary, FuncCode::Assign);
}

AndNode::AndNode(Token token, 
        std::unique_ptr<ExpressionNode> left, 
        std::unique_ptr<ExpressionNode> right)
        : BinaryExpressionNode(token, std::move(left), std::move(right)) {}


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

OrNode::OrNode(Token token, 
        std::unique_ptr<ExpressionNode> left, 
        std::unique_ptr<ExpressionNode> right)
        : BinaryExpressionNode(token, std::move(left), std::move(right)) {}

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

SubscriptNode::SubscriptNode(
        std::unique_ptr<ExpressionNode> left, 
        std::unique_ptr<ExpressionNode> right)
        : BinaryExpressionNode(Token::synthetic("<subscript>"), 
        std::move(left), std::move(right)) {}

bool SubscriptNode::is_lvalue() const {
    return true;
}

void SubscriptNode::serialize(Serializer &serializer) const {
    serialize_load_address(serializer);
    serializer.add_instr(OpCode::LoadAbs);
}

void SubscriptNode::serialize_load_address(Serializer &serializer) const {
    m_left->serialize(serializer);
    m_right->serialize(serializer);
    serializer.add_instr(OpCode::Binary, FuncCode::Add);
}

CallNode::CallNode(
        std::unique_ptr<ExpressionNode> func, 
        std::unique_ptr<ExpressionListNode> args)
        : ExpressionNode(Token::synthetic("<call>")), m_func(std::move(func)), 
        m_args(std::move(args)) {}

std::unique_ptr<CallNode> CallNode::make_call(Token ident, 
        std::vector<std::unique_ptr<ExpressionNode>> params) {
    return std::make_unique<CallNode>(
            std::make_unique<VariableNode>(ident), 
            std::make_unique<ExpressionListNode>(
                Token::synthetic("<params>"), std::move(params)));
}

std::unique_ptr<CallNode> CallNode::make_unary_call(Token ident, 
        std::unique_ptr<ExpressionNode> param) {
    std::vector<std::unique_ptr<ExpressionNode>> params;
    params.push_back(std::move(param));
    return CallNode::make_call(ident, std::move(params));
}

std::unique_ptr<CallNode> CallNode::make_binary_call(Token ident, 
        std::unique_ptr<ExpressionNode> left, 
        std::unique_ptr<ExpressionNode> right) {
    std::vector<std::unique_ptr<ExpressionNode>> params;
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
    SymbolEntry const &entry = serializer.symbol_table().get(id);
    if (entry.storage_type == StorageType::Intrinsic) {
        IntrinsicEntry intrinsic = intrinsics[entry.value];
        if (m_args->exprs().size() != intrinsic.n_args) {
            throw std::runtime_error(
                    "Invalid intrinsic invocation of " + intrinsic.symbol);
        }
        m_args->serialize(serializer);
        serializer.add_instr(intrinsic.opcode, intrinsic.funccode);
    } else if (entry.storage_type == StorageType::Callable) {
        serializer.call(id, m_args->exprs());
    } else {
        m_args->serialize(serializer);
        serializer.add_instr(OpCode::Push, m_args->exprs().size());
        m_func->serialize(serializer);
        serializer.add_instr(OpCode::Call);
    }
}

void CallNode::print(TreePrinter &printer) const {
    printer.print_node(this);
    printer.next_child(m_func.get());
    printer.last_child(m_args.get());
}

TernaryNode::TernaryNode(Token token, std::unique_ptr<ExpressionNode> cond, 
        std::unique_ptr<ExpressionNode> case_true, 
        std::unique_ptr<ExpressionNode> case_false)
        : ExpressionNode(token), m_cond(std::move(cond)), 
        m_case_true(std::move(case_true)), 
        m_case_false(std::move(case_false)) {}

void TernaryNode::resolve_locals(Serializer &serializer, 
        ScopeTracker &scopes) {
    m_cond->resolve_locals(serializer, scopes);
    m_case_true->resolve_locals(serializer, scopes);
    m_case_false->resolve_locals(serializer, scopes);
}

void TernaryNode::serialize(Serializer &serializer) const {
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

void TernaryNode::print(TreePrinter &printer) const {
    printer.print_node(this);
    printer.next_child(m_cond.get());
    printer.next_child(m_case_true.get());
    printer.last_child(m_case_false.get());
}

LambdaNode::LambdaNode(Token token, 
        CallableSignature signature, 
        std::unique_ptr<StatementNode> body)
        : ExpressionNode(token), m_body(std::move(body)),
        m_signature(std::move(signature)) {}

void LambdaNode::resolve_locals(Serializer &serializer, ScopeTracker &scopes) {
    ScopeTracker block_scopes(scopes.global, scopes.enclosing, {});
    uint32_t position = -3 - m_signature.params.size();

    for (std::size_t i = 0; i < m_signature.params.size(); i++) {
        Token const &token = m_signature.params[i];
        TypeNode *type = m_signature.type->param_types()->list()[i].get();

        serializer.symbol_table().declare(block_scopes.current, token.data(), 
                this, type, StorageType::Relative, position);

        position++;
    }
    
    m_body->resolve_locals(serializer, block_scopes);
}

void LambdaNode::serialize(Serializer &serializer) const {
    SymbolId id = serializer.get_label();
    serializer.add_job(id, m_body.get(), false);
    serializer.add_instr(OpCode::Push, id, true);
}

void LambdaNode::print(TreePrinter &printer) const {
    printer.print_node(this);
    printer.next_child(m_signature.type.get());
    printer.last_child(m_body.get());
}

std::string LambdaNode::label() const {
    return token().data() + " (" + 
            tokenlist_to_string(m_signature.params) + ")";
}

CallableNode::CallableNode(Token token, Token ident, 
        CallableSignature signature, std::unique_ptr<BaseNode> body)
        : StatementNode(token), m_body(std::move(body)), m_ident(ident),
        m_signature(std::move(signature)) {}

bool CallableNode::is_matching_call(
        std::vector<std::unique_ptr<ExpressionNode>> const &args) const {
    return args.size() == n_params();
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

CallableSignature const &CallableNode::signature() const {
    return m_signature;
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

    for (std::size_t i = 0; i < n_params(); i++) {
        Token const &token = params()[i];
        TypeNode *type = signature().type->param_types()->list()[i].get();
        serializer.symbol_table().declare(block_scopes.current, token.data(), 
                this, type, StorageType::Relative, position);
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
        std::vector<std::unique_ptr<ExpressionNode>> const &args) const {
    for (auto const &node : args) {
        node->serialize(serializer);
    }
    serializer.add_instr(OpCode::Push, n_params());
    serializer.add_instr(OpCode::Push, id(), true);
    serializer.add_instr(OpCode::Call);
}

void FunctionNode::print(TreePrinter &printer) const {
    printer.print_node(this);
    printer.next_child(m_signature.type.get());
    printer.last_child(m_body.get());
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

    for (std::size_t i = 0; i < n_params(); i++) {
        Token const &token = params()[i];
        TypeNode *type = signature().type->param_types()->list()[i].get();

        SymbolId id = serializer.symbol_table().declare(block_scopes.current, 
                token.data(), this, type, StorageType::InlineReference, 
                position);
        m_param_ids.push_back(id);

        position++;
    }

    m_body->resolve_locals(serializer, block_scopes);
}

void InlineNode::serialize(Serializer &) const {}

void InlineNode::serialize_call(Serializer &serializer, 
        std::vector<std::unique_ptr<ExpressionNode>> const &args) const {
    serializer.inline_frames().open_call(args, m_param_ids);
    m_body->serialize(serializer);
    serializer.inline_frames().close_call(m_param_ids);
}

void InlineNode::print(TreePrinter &printer) const {
    printer.print_node(this);
    printer.next_child(m_signature.type.get());
    printer.last_child(m_body.get());
}

std::string InlineNode::label() const {
    return token().data() + " " + ident().data() + "(" 
            + tokenlist_to_string(params()) + ")";
}

EmptyNode::EmptyNode()
        : StatementNode(Token::synthetic("<empty>")) {}

void EmptyNode::resolve_globals(Serializer &, SymbolMap &) {}

void EmptyNode::resolve_locals(Serializer &, ScopeTracker &) {}

void EmptyNode::serialize(Serializer &) const {}

void EmptyNode::print(TreePrinter &printer) const {
    printer.print_node(this);
}

BlockNode::BlockNode(std::vector<std::unique_ptr<StatementNode>> statements)
        : StatementNode(Token::synthetic("<block>")), 
        m_statements(std::move(statements)), m_scope_map() {}

void BlockNode::resolve_globals(
        Serializer &serializer, SymbolMap &symbol_map) {
    for (auto const &stmt : m_statements) {
        stmt->resolve_globals(serializer, symbol_map);
    }
}

void BlockNode::resolve_locals(Serializer &serializer, 
        ScopeTracker &scopes) {
    for (auto const &stmt : m_statements) {
        stmt->resolve_locals(serializer, scopes);
    }
}

void BlockNode::serialize(Serializer &serializer) const {
    for (std::unique_ptr<StatementNode> const &stmt : m_statements) {
        stmt->serialize(serializer);
    }
}

void BlockNode::print(TreePrinter &printer) const {
    printer.print_node(this);
    for (std::size_t i = 0; i < m_statements.size(); i++) {
        if (i == m_statements.size() - 1) {
            printer.last_child(m_statements[i].get());
        } else {
            printer.next_child(m_statements[i].get());
        }
    }
}

ScopedBlockNode::ScopedBlockNode(
        std::vector<std::unique_ptr<StatementNode>> statements)
        : StatementNode(Token::synthetic("<scoped-block>")), 
        m_statements(std::move(statements)), m_scope_map() {}

void ScopedBlockNode::resolve_globals(Serializer &, SymbolMap &) {}

void ScopedBlockNode::resolve_locals(Serializer &serializer, 
        ScopeTracker &scopes) {
    ScopeTracker block_scopes(scopes.global, scopes.enclosing, {});
    for (auto const &entry : scopes.current) {
        block_scopes.enclosing[entry.first] = entry.second;
    }
    for (auto const &stmt : m_statements) {
        stmt->resolve_locals(serializer, block_scopes);
    }
}

void ScopedBlockNode::serialize(Serializer &serializer) const {
    for (auto const &stmt : m_statements) {
        stmt->serialize(serializer);
    }
}

void ScopedBlockNode::print(TreePrinter &printer) const {
    printer.print_node(this);
    for (std::size_t i = 0; i < m_statements.size(); i++) {
        if (i == m_statements.size() - 1) {
            printer.last_child(m_statements[i].get());
        } else {
            printer.next_child(m_statements[i].get());
        }
    }
}

TypeDeclarationNode::TypeDeclarationNode(
        Token token, std::unique_ptr<NamedTypeNode> ident)
        : StatementNode(token), m_ident(std::move(ident)) {}

void TypeDeclarationNode::resolve_globals(
        Serializer &serializer, SymbolMap &symbol_map) {
    set_id(serializer.symbol_table().declare(symbol_map, 
            m_ident->token().data(), this, nullptr, StorageType::Type));
}

void TypeDeclarationNode::resolve_locals(Serializer &, ScopeTracker &) {}

void TypeDeclarationNode::serialize(Serializer &) const {}

void TypeDeclarationNode::print(TreePrinter &printer) const {
    printer.print_node(this);
}

std::string TypeDeclarationNode::label() const {
    return token().data() + " " + m_ident->token().data();
}

ExpressionListNode::ExpressionListNode(
        Token token, std::vector<std::unique_ptr<ExpressionNode>> exprs)
        : BaseNode(token), m_exprs(std::move(exprs)) {}

void ExpressionListNode::resolve_globals(Serializer &, SymbolMap &) {}

void ExpressionListNode::resolve_locals(Serializer &serializer, 
        ScopeTracker &scopes) {
    for (auto const &expr : m_exprs) {
        expr->resolve_locals(serializer, scopes);
    }
}

void ExpressionListNode::serialize(Serializer &serializer) const {
    for (auto const &expr : m_exprs) {
        expr->serialize(serializer);
    }
}

void ExpressionListNode::print(TreePrinter &printer) const {
    printer.print_node(this);
    for (std::size_t i = 0; i < m_exprs.size(); i++) {
        if (i == m_exprs.size() - 1) {
            printer.last_child(m_exprs[i].get());
        } else {
            printer.next_child(m_exprs[i].get());
        }
    }
}

std::vector<std::unique_ptr<ExpressionNode>> const &
        ExpressionListNode::exprs() {
    return m_exprs;
}

IfNode::IfNode(Token token, std::unique_ptr<ExpressionNode> cond, 
        std::unique_ptr<StatementNode> case_true)
        : StatementNode(token), m_cond(std::move(cond)), 
        m_case_true(std::move(case_true)) {}

void IfNode::resolve_globals(Serializer &, SymbolMap &) {}

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

void IfNode::print(TreePrinter &printer) const {
    printer.print_node(this);
    printer.next_child(m_cond.get());
    printer.last_child(m_case_true.get());
}


IfElseNode::IfElseNode(Token token, std::unique_ptr<ExpressionNode> cond, 
        std::unique_ptr<StatementNode> case_true, 
        std::unique_ptr<StatementNode> case_false)
        : StatementNode(token), m_cond(std::move(cond)), 
        m_case_true(std::move(case_true)), 
        m_case_false(std::move(case_false)) {}

void IfElseNode::resolve_globals(Serializer &, SymbolMap &) {}

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

void IfElseNode::print(TreePrinter &printer) const {
    printer.print_node(this);
    printer.next_child(m_cond.get());
    printer.next_child(m_case_true.get());
    printer.last_child(m_case_false.get());
}

ForLoopNode::ForLoopNode(Token token, 
        std::unique_ptr<StatementNode> init, 
        std::unique_ptr<ExpressionNode> cond, 
        std::unique_ptr<StatementNode> post, 
        std::unique_ptr<StatementNode> body)
        : StatementNode(token), 
        m_init(std::move(init)), m_cond(std::move(cond)), 
        m_post(std::move(post)), m_body(std::move(body)) {}

void ForLoopNode::resolve_globals(Serializer &, SymbolMap &) {}

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

void ForLoopNode::print(TreePrinter &printer) const {
    printer.print_node(this);
    printer.next_child(m_init.get());
    printer.next_child(m_cond.get());
    printer.next_child(m_post.get());
    printer.last_child(m_body.get());
}

ReturnNode::ReturnNode(Token token, std::unique_ptr<ExpressionNode> operand)
        : StatementNode(token), m_operand(std::move(operand)) {}

void ReturnNode::resolve_globals(Serializer &, SymbolMap &) {}

void ReturnNode::resolve_locals(Serializer &serializer, 
        ScopeTracker &scopes) {
    m_operand->resolve_locals(serializer, scopes);
}

void ReturnNode::serialize(Serializer &serializer) const {
    m_operand->serialize(serializer);
    serializer.add_instr(OpCode::Ret);
}

void ReturnNode::print(TreePrinter &printer) const {
    printer.print_node(this);
    printer.last_child(m_operand.get());
}

VarDeclarationNode::VarDeclarationNode(Token token, Token ident, 
        std::unique_ptr<TypeNode> type,
        std::unique_ptr<ExpressionNode> size, 
        std::unique_ptr<ExpressionNode> init_value)
        : StatementNode(token), m_ident(ident), m_type(std::move(type)),
        m_size(std::move(size)), 
        m_init_value(std::move(init_value)) {}

void VarDeclarationNode::resolve_globals(
        Serializer &serializer, SymbolMap &current) {
    if (m_init_value != nullptr) {
        throw std::runtime_error("not implemented");
    }
    set_id(serializer.symbol_table().declare(current, m_ident.data(), 
            this, m_type.get(), 
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
    set_id(serializer.symbol_table().declare(scopes.current, m_ident.data(), 
            this, m_type.get(), 
            m_size == nullptr ? 
                StorageType::Relative : StorageType::RelativeRef, 
            0, declared_size()));
    serializer.symbol_table().add_to_container(id());
}

void VarDeclarationNode::serialize(Serializer &serializer) const {
    if (m_init_value != nullptr) {
        SymbolEntry const &entry = serializer.symbol_table().get(id());
        serializer.add_instr(OpCode::LoadAddrRel, entry.value);
        m_init_value->serialize(serializer);
        serializer.add_instr(OpCode::Binary, FuncCode::Assign);
        serializer.add_instr(OpCode::Pop);
    }
}

void VarDeclarationNode::print(TreePrinter &printer) const {
    printer.print_node(this);
    printer.next_child(m_size.get());
    printer.last_child(m_init_value.get());
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

ExpressionStatementNode::ExpressionStatementNode(
        std::unique_ptr<ExpressionNode> expr)
        : StatementNode(Token::synthetic("<expr-stmt>")), 
        m_expr(std::move(expr)) {}

void ExpressionStatementNode::resolve_globals(Serializer &, SymbolMap &) {}

void ExpressionStatementNode::resolve_locals(Serializer &serializer, 
        ScopeTracker &scopes) {
    m_expr->resolve_locals(serializer, scopes);
}

void ExpressionStatementNode::serialize(Serializer &serializer) const {
    m_expr->serialize(serializer);
    serializer.add_instr(OpCode::Pop);
}

void ExpressionStatementNode::print(TreePrinter &printer) const {
    printer.print_node(this);
    printer.last_child(m_expr.get());
}
