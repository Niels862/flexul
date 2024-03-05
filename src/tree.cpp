#include "tree.hpp"
#include "utils.hpp"
#include <iostream>

BaseNode::BaseNode(Token token, std::vector<BaseNode *> children)
        : m_token(token), m_children(children), m_id(0) {}

BaseNode::~BaseNode() {
    for (BaseNode const *child : m_children) {
        delete child;
    }
}

bool BaseNode::is_lvalue() const {
    return false;
}

void BaseNode::resolve_symbols_first_pass(Serializer &, SymbolMap &) {}

void BaseNode::resolve_symbols_second_pass(
        Serializer &serializer, SymbolMap &global_scope, 
        SymbolMap &enclosing_scope, SymbolMap &current_scope) {
    for (BaseNode *child : children()) {
        if (child == nullptr) {
            continue; // todo fix better
        }
        child->resolve_symbols_second_pass(serializer, global_scope, 
                enclosing_scope, current_scope);
    }
}

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

const std::vector<BaseNode *> &BaseNode::children() const {
    return m_children;
}

BaseNode *BaseNode::get(size_t n) const {
    if (n >= m_children.size()) {
        throw std::runtime_error("Cannot access child: " + std::to_string(n));
    }
    return m_children[n];
}

void BaseNode::set_id(SymbolId id) {
    m_id = id;
}

SymbolId BaseNode::id() const {
    return m_id;
}

void BaseNode::print(BaseNode *node,
        std::string const &label_prefix, 
        std::string const &branch_prefix) {
    size_t i;
    if (node == nullptr) {
        std::cerr << label_prefix << "(null)" << std::endl;
        return;
    }
    std::cerr << label_prefix << node->label();
    if (node->id() != 0) {
        std::cerr << " (" << node->id() << ")";
    }
    std::cerr << std::endl;
    if (node->children().empty()) {
        return;
    }
    for (i = 0; i < node->children().size() - 1; i++) {
        BaseNode::print(node->children()[i],
                branch_prefix + "\u251C\u2500", 
                branch_prefix + "\u2502 ");
    }
    BaseNode::print(node->children()[i],
            branch_prefix + "\u2570\u2500", 
            branch_prefix + "  ");
}

TypeNode::TypeNode(Token token, std::vector<BaseNode *> children)
        : BaseNode(token, children) {}

void TypeNode::serialize(Serializer &) const {}

NamedTypeNode::NamedTypeNode(Token ident)
        : TypeNode(ident, {}) {}

TypeListNode::TypeListNode(std::vector<TypeNode *> type_list)
        : TypeNode(Token::synthetic("<type-list>"), 
        derive_vector<BaseNode *>(type_list)), m_type_list(type_list) {}

CallableTypeNode::CallableTypeNode(Token token, TypeListNode *param_types, 
        TypeNode *return_type)
        : TypeNode(token, {param_types, return_type}) {}

EmptyNode::EmptyNode()
        : BaseNode(Token::synthetic("<empty>"), {}) {}

void EmptyNode::serialize(Serializer &) const {}

IntLitNode::IntLitNode(Token token)
        : BaseNode(token, {}), m_value(token.to_int()) {
}

void IntLitNode::serialize(Serializer &serializer) const {
    serializer.add_instr(OpCode::Push, m_value);
}

std::optional<uint32_t> IntLitNode::get_constant_value() const {
    return m_value;
}

VariableNode::VariableNode(Token token)
        : BaseNode(token, {}) {}

bool VariableNode::is_lvalue() const {
    return true;
}

void VariableNode::resolve_symbols_second_pass(
        Serializer &, SymbolMap &global_scope, 
        SymbolMap &enclosing_scope, SymbolMap &current_scope) {
    SymbolId id = lookup_symbol(token().data(), global_scope, 
            enclosing_scope, current_scope);
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

AssignNode::AssignNode(Token token, BaseNode *left, BaseNode *right)
        : BaseNode(token, {left, right}) {
    if (!left->is_lvalue()) {
        throw std::runtime_error("Expected lvalue as assignment target");
    }
}

void AssignNode::serialize(Serializer &serializer) const {
    get(Target)->serialize_load_address(serializer);
    get(Expr)->serialize(serializer);
    serializer.add_instr(OpCode::Binary, FuncCode::Assign);
}

AndNode::AndNode(Token token, BaseNode *left, BaseNode *right)
        : BaseNode(token, {left, right}) {}

void AndNode::serialize(Serializer &serializer) const {
    Label label_false = serializer.get_label();
    Label label_end = serializer.get_label();

    get(Left)->serialize(serializer);
    serializer.add_instr(OpCode::BrFalse, label_false, true);

    get(Right)->serialize(serializer);
    serializer.add_instr(OpCode::BrFalse, label_false, true);
    serializer.add_instr(OpCode::Push, 1);
    serializer.add_instr(OpCode::Jump, label_end, true);

    serializer.add_label(label_false);
    serializer.add_instr(OpCode::Push, 0);

    serializer.add_label(label_end);
}

OrNode::OrNode(Token token, BaseNode *left, BaseNode *right)
        : BaseNode(token, {left, right}) {}

void OrNode::serialize(Serializer &serializer) const {
    Label label_true = serializer.get_label();
    Label label_end = serializer.get_label();

    get(Left)->serialize(serializer);
    serializer.add_instr(OpCode::BrTrue, label_true, true);

    get(Right)->serialize(serializer);
    serializer.add_instr(OpCode::BrTrue, label_true, true);
    serializer.add_instr(OpCode::Push, 0);
    serializer.add_instr(OpCode::Jump, label_end, true);

    serializer.add_label(label_true);
    serializer.add_instr(OpCode::Push, 1);

    serializer.add_label(label_end);
}

AddressOfNode::AddressOfNode(Token token, BaseNode *operand)
        : BaseNode(token, {operand}) {}

void AddressOfNode::serialize(Serializer &serializer) const {
    get(Operand)->serialize_load_address(serializer);
}

DereferenceNode::DereferenceNode(Token token, BaseNode *operand)
        : BaseNode(token, {operand}) {}

bool DereferenceNode::is_lvalue() const {
    return true;
}

void DereferenceNode::serialize(Serializer &serializer) const {
    get(Operand)->serialize(serializer);
    serializer.add_instr(OpCode::LoadAbs);
}

void DereferenceNode::serialize_load_address(Serializer &serializer) const {
    get(Operand)->serialize(serializer);
}

IfElseNode::IfElseNode(Token token, BaseNode *cond, BaseNode *case_true, 
        BaseNode *case_false)
        : BaseNode(token, {cond, case_true, case_false}) {}

void IfElseNode::serialize(Serializer &serializer) const {
    Label label_false = serializer.get_label();
    Label label_end = serializer.get_label();
    
    get(Cond)->serialize(serializer);
    serializer.add_instr(OpCode::BrFalse, label_false, true);

    get(CaseTrue)->serialize(serializer);
    serializer.add_instr(OpCode::Jump, label_end, true);

    serializer.add_label(label_false);
    get(CaseFalse)->serialize(serializer);

    serializer.add_label(label_end);
}

CallNode::CallNode(BaseNode *func, BaseNode *args)
        : BaseNode(Token::synthetic("<call>"), {func, args}) {}

std::unique_ptr<BaseNode> CallNode::make_call(Token ident, 
        std::vector<std::unique_ptr<BaseNode>> params) {
    return std::make_unique<CallNode>(
            std::make_unique<VariableNode>(token), 
            std::make_unique<ExpressionListNode>(
                Token::synthetic("<params>"), params));
}

std::unique_ptr<BaseNode> CallNode::make_unary_call(Token ident, 
        std::unique_ptr<BaseNode> param) {
    std::vector<std::unique_ptr<BaseNode>> params = 
            {std::move(param)};
    return CallNode::make_call(ident, params);
}

std::unique_ptr<BaseNode> CallNode::make_binary_call(Token ident, 
        std::unique_ptr<BaseNode> left, 
        std::unique_ptr<BaseNode> right) {
    std::vector<std::unique_ptr<BaseNode>> params = 
            {std::move(left), std::move(right)};
    return CallNode::make_call(ident, params);
}

void CallNode::serialize(Serializer &serializer) const {
    SymbolId id = get(Func)->id();
    SymbolEntry entry = serializer.symbol_table().get(id);
    if (entry.storage_type == StorageType::Intrinsic) {
        IntrinsicEntry intrinsic = intrinsics[entry.value];
        if (get(Args)->children().size() != intrinsic.n_args) {
            throw std::runtime_error(
                    "Invalid intrinsic invocation of " + intrinsic.symbol);
        }
        get(Args)->serialize(serializer);
        serializer.add_instr(intrinsic.opcode, intrinsic.funccode);
    } else if (entry.storage_type == StorageType::Callable) {
        serializer.call(id, get(Args));
    } else {
        get(Args)->serialize(serializer);
        serializer.add_instr(OpCode::Push, get(Args)->children().size());
        get(Func)->serialize(serializer);
        serializer.add_instr(OpCode::Call);
    }
}

SubscriptNode::SubscriptNode(BaseNode *array, BaseNode *subscript)
        : BaseNode(Token::synthetic("<subscript>"), {array, subscript}) {}

bool SubscriptNode::is_lvalue() const {
    return true;
}

void SubscriptNode::serialize(Serializer &serializer) const {
    serialize_load_address(serializer);
    serializer.add_instr(OpCode::LoadAbs);
}

void SubscriptNode::serialize_load_address(Serializer &serializer) const {
    get(Array)->serialize(serializer);
    get(Subscript)->serialize(serializer);
    serializer.add_instr(OpCode::Binary, FuncCode::Add);
}

BlockNode::BlockNode(std::vector<BaseNode *> children)
        : BaseNode(Token::synthetic("<block>"), children), m_scope_map() {}

void BlockNode::resolve_symbols_first_pass(
        Serializer &serializer, SymbolMap &symbol_map) {
    for (BaseNode *child : children()) {
        child->resolve_symbols_first_pass(serializer, symbol_map);
    }
}

void BlockNode::serialize(Serializer &serializer) const {
    for (BaseNode const *child : children()) {
        child->serialize(serializer);
    }
}

ScopedBlockNode::ScopedBlockNode(std::vector<BaseNode *> children)
        : BaseNode(Token::synthetic("<scoped-block>"), children), m_scope_map() {}

void ScopedBlockNode::resolve_symbols_second_pass(
        Serializer &serializer, SymbolMap &global_scope, 
        SymbolMap &enclosing_scope, SymbolMap &current_scope) {
    SymbolMap block_scope;
    SymbolMap block_enclosing_scope = enclosing_scope;
    for (auto const &key_value : current_scope) {
        block_enclosing_scope[key_value.first] = key_value.second;
    }
    for (BaseNode *child : children()) {
        child->resolve_symbols_second_pass(serializer, global_scope, 
                block_enclosing_scope, block_scope);
    }
}

void ScopedBlockNode::serialize(Serializer &serializer) const {
    for (BaseNode const *child : children()) {
        child->serialize(serializer);
    }
}

CallableNode::CallableNode(Token token, Token ident, 
        CallableSignature signature, BaseNode *body)
        : BaseNode(token, std::vector<BaseNode *>(
            {&*signature.type, body})), m_ident(ident), 
        m_signature(std::move(signature)) {}

bool CallableNode::is_matching_call(BaseNode *params) const {
    return params->children().size() == n_params();
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
        CallableSignature signature, BaseNode *body)
        : CallableNode(token, ident, std::move(signature), body) {}

void FunctionNode::resolve_symbols_first_pass(
        Serializer &serializer, SymbolMap &symbol_map) {
    set_id(serializer.declare_callable(ident().data(), 
            symbol_map, this));
    serializer.add_job(id(), this, false);
}

void FunctionNode::resolve_symbols_second_pass(
        Serializer &serializer, SymbolMap &global_scope, 
        SymbolMap &, SymbolMap &) {
    // used as enclosing scope which is empty
    // for function
    SymbolMap empty_scope;
    SymbolMap function_scope;
    uint32_t position = -3 - n_params();
    for (Token const &param : params()) {
        serializer.symbol_table().declare(param.data(), function_scope, 
                StorageType::Relative, position);
        position++;
    }
    serializer.symbol_table().open_container();
    get(Body)->resolve_symbols_second_pass(serializer, global_scope, 
            empty_scope, function_scope);
    m_frame_size = serializer.symbol_table().container_size();
    serializer.symbol_table().resolve_local_container();
}

void FunctionNode::serialize(Serializer &serializer) const {
    if (id() == 0) {
        throw std::runtime_error("Unresolved name");
    }
    serializer.add_instr(OpCode::AddSp, m_frame_size);
    get(Body)->serialize(serializer);
    serializer.add_instr(OpCode::Ret, 0);
}

void FunctionNode::serialize_call(Serializer &serializer, 
        BaseNode *params) const {
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
        CallableSignature signature, BaseNode *body)
        : CallableNode(token, ident, std::move(signature), body), m_param_ids() {}

void InlineNode::resolve_symbols_first_pass(
        Serializer &serializer, SymbolMap &symbol_map) {
    set_id(serializer.declare_callable(ident().data(), 
            symbol_map, this));
    serializer.add_job(id(), this, true);
}

void InlineNode::resolve_symbols_second_pass(
        Serializer &serializer, SymbolMap &global_scope, 
        SymbolMap &, SymbolMap &) {
    SymbolMap empty_scope;
    SymbolMap function_scope;
    uint32_t position = 0;
    for (Token const &param : params()) {
        SymbolId id = serializer.symbol_table().declare(param.data(), 
                function_scope, StorageType::InlineReference, position);
        m_param_ids.push_back(id);
        position++;
    }
    get(Body)->resolve_symbols_second_pass(serializer, global_scope, 
            empty_scope, function_scope);
}

void InlineNode::serialize(Serializer &) const {}

void InlineNode::serialize_call(Serializer &serializer, 
        BaseNode *params) const {
    serializer.inline_frames().open_call(params, m_param_ids);
    get(Body)->serialize(serializer);
    serializer.inline_frames().close_call(m_param_ids);
}

std::string InlineNode::get_label() const {
    return token().data() + " " + ident().data() + "(" 
            + tokenlist_to_string(params()) + ")";
}

LambdaNode::LambdaNode(Token token, 
        CallableSignature signature, BaseNode *expr)
        : CallableNode(
            token, Token::synthetic("<anonymous>"), 
            std::move(signature), {expr}) {}

void LambdaNode::resolve_symbols_second_pass(
        Serializer &serializer, SymbolMap &global_scope, 
        SymbolMap &, SymbolMap &) {
    SymbolMap empty_scope;
    SymbolMap lambda_scope;
    uint32_t position = -3 - n_params();
    for (Token const &param : params()) {
        serializer.symbol_table().declare(param.data(), lambda_scope, 
                StorageType::Relative, position);
        position++;
    }
    get(Body)->resolve_symbols_second_pass(serializer, global_scope, 
            empty_scope, lambda_scope);
}

void LambdaNode::serialize(Serializer &serializer) const {
    SymbolId id = serializer.get_label();
    serializer.add_job(id, get(Body), false);
    serializer.add_instr(OpCode::Push, id, true);
}

void LambdaNode::serialize_call(Serializer &, BaseNode *) const {}

std::string LambdaNode::label() const {
    return token().data() + " (" + tokenlist_to_string(params()) + ")";
}

TypeDeclarationNode::TypeDeclarationNode(
        Token token, Token ident)
        : BaseNode(token, {}), m_ident(ident) {}

void TypeDeclarationNode::resolve_symbols_first_pass(
        Serializer &serializer, SymbolMap &symbol_map) {
    set_id(serializer.symbol_table().declare(
            m_ident.data(), symbol_map, StorageType::Type));
}

void TypeDeclarationNode::serialize(Serializer &) const {}

std::string TypeDeclarationNode::label() const {
    return token().data() + " " + m_ident.data();
}

ExpressionListNode::ExpressionListNode(
        Token token, std::vector<BaseNode *> children)
        : BaseNode(token, children) {}

void ExpressionListNode::serialize(Serializer &serializer) const {
    for (BaseNode const *child : children()) {
        child->serialize(serializer);
    }
}

IfNode::IfNode(Token token, BaseNode *cond, BaseNode *case_true)
        : BaseNode(token, {cond, case_true}) {}

void IfNode::serialize(Serializer &serializer) const {
    Label label_end = serializer.get_label();
    get(Cond)->serialize(serializer);
    serializer.add_instr(OpCode::BrFalse, label_end, true);
    get(CaseTrue)->serialize(serializer);
    serializer.add_label(label_end);
}

ForLoopNode::ForLoopNode(Token token, BaseNode *init, BaseNode *cond, 
        BaseNode *post, BaseNode *body)
        : BaseNode(token, {init, cond, post, body}) {}

void ForLoopNode::serialize(Serializer &serializer) const {
    Label loop_body_label = serializer.get_label();
    Label cond_label = serializer.get_label();

    get(Init)->serialize(serializer); // init: statement
    serializer.add_instr(OpCode::Jump, cond_label, true);
    
    serializer.add_label(loop_body_label); // body and post: statements
    get(Body)->serialize(serializer);
    get(Post)->serialize(serializer);

    serializer.add_label(cond_label);
    get(Cond)->serialize(serializer); // cond: expression
    serializer.add_instr(OpCode::BrTrue, loop_body_label, true);
}

ReturnNode::ReturnNode(Token token, BaseNode *child)
        : BaseNode(token, {child}) {}

void ReturnNode::serialize(Serializer &serializer) const {
    get(RetValue)->serialize(serializer);
    serializer.add_instr(OpCode::Ret);
}

VarDeclarationNode::VarDeclarationNode(Token token, Token ident, BaseNode *size, 
        BaseNode *init_value)
        : BaseNode(token, {size, init_value}), m_ident(ident) {}

void VarDeclarationNode::resolve_symbols_first_pass(
        Serializer &serializer, SymbolMap &current_scope) {
    if (get(InitValue) != nullptr) {
        throw std::runtime_error("not implemented");
    }
    set_id(serializer.symbol_table().declare(
            m_ident.data(), current_scope, 
            get(Size) == nullptr ? 
                StorageType::Absolute : StorageType::AbsoluteRef, 
            0, declared_size()));
    serializer.symbol_table().add_to_container(id());
}

void VarDeclarationNode::resolve_symbols_second_pass(
        Serializer &serializer, SymbolMap &global_scope, 
        SymbolMap &enclosing_scope, SymbolMap &current_scope) {
    if (get(InitValue) != nullptr) {
        get(InitValue)->resolve_symbols_second_pass(serializer, global_scope, 
                enclosing_scope, current_scope);
    }
    set_id(serializer.symbol_table().declare(
            m_ident.data(), current_scope, 
            get(Size) == nullptr ? 
                StorageType::Relative : StorageType::RelativeRef, 
            0, declared_size()));
    serializer.symbol_table().add_to_container(id());
}

void VarDeclarationNode::serialize(Serializer &serializer) const {
    if (get(InitValue) != nullptr) {
        SymbolEntry entry = serializer.symbol_table().get(id());
        serializer.add_instr(OpCode::LoadAddrRel, entry.value);
        get(InitValue)->serialize(serializer);
        serializer.add_instr(OpCode::Binary, FuncCode::Assign);
        serializer.add_instr(OpCode::Pop);
    }
}

std::string VarDeclarationNode::label() const {
    return token().data() + " " + m_ident.data();
}

uint32_t VarDeclarationNode::declared_size() const {
    if (get(Size) == nullptr) {
        return 1;
    }
    std::optional<uint32_t> const_value = get(Size)->get_constant_value();
    if (const_value.has_value()) {
        return const_value.value();
    }
    throw std::runtime_error("Expected constant value as size");
}

ExpressionStatementNode::ExpressionStatementNode(BaseNode *child)
        : BaseNode(Token::synthetic("<expr-stmt>"), {child}) {}

void ExpressionStatementNode::serialize(Serializer &serializer) const {
    get(Expr)->serialize(serializer);
    serializer.add_instr(OpCode::Pop);
}
