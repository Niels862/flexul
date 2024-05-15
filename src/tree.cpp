#include "tree.hpp"
#include "treeprinter.hpp"
#include "utils.hpp"
#include <iostream>

AnyTypeNode Any;

TypeMatch weakest_match(TypeMatch m1, TypeMatch m2) {
    return static_cast<TypeMatch>(std::min(
            static_cast<int>(m1), 
            static_cast<int>(m2)));
}

BaseNode::BaseNode(Token token)
        : m_token(token), m_id(0) {}

BaseNode::~BaseNode() {}

bool BaseNode::is_lvalue() const {
    return false;
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

void BaseNode::set_id(SymbolId id) {
    m_id = id;
}

SymbolId BaseNode::id() const {
    return m_id;
}

TypeNode::TypeNode(Token token)
        : BaseNode(token) {}

void TypeNode::resolve_globals(SymbolTable &, SymbolMap &) {}

void TypeNode::resolve_types(SymbolTable &) {}

void TypeNode::serialize(Serializer &) const {}

std::string to_string(TypeNode const *node) {
    if (node == nullptr) {
        return "(null)";
    }
    return node->type_string();
}

AnyTypeNode::AnyTypeNode()
        : TypeNode(Token::synthetic("<Any>")) {}

void AnyTypeNode::resolve_locals(SymbolTable &, ScopeTracker &) {}

TypeMatch AnyTypeNode::matching(TypeNode const *node) const {
    if (dynamic_cast<AnyTypeNode const *>(node) != nullptr) {
        return TypeMatch::ExactMatch;
    }
    return TypeMatch::AnyMatch;
}

TypeNode const *AnyTypeNode::called_type() const {
    return this;
}

TypeNode const *AnyTypeNode::pointed_type() const {
    return this;
}

void AnyTypeNode::print(TreePrinter &printer) const {
    printer.print_node(this);
}

std::string AnyTypeNode::type_string() const {
    return "Any";
}

NamedTypeNode::NamedTypeNode(Token ident)
        : TypeNode(ident) {}

void NamedTypeNode::resolve_locals(SymbolTable &, ScopeTracker &scopes) {
    set_id(lookup_symbol(token().data(), scopes));
}

TypeMatch NamedTypeNode::matching(TypeNode const *node) const {
    if (dynamic_cast<AnyTypeNode const *>(node) != nullptr) {
        return TypeMatch::AnyMatch;
    }
    NamedTypeNode const *other = dynamic_cast<NamedTypeNode const *>(node);
    if (other == nullptr || token() != other->token()) {
        return TypeMatch::NoMatch;
    }
    return TypeMatch::ExactMatch;
}

TypeNode const *NamedTypeNode::called_type() const {
    throw std::runtime_error("Cannot call '" + token().data() + "'");
}

TypeNode const *NamedTypeNode::pointed_type() const {
    throw std::runtime_error("No pointed type for '" + token().data() + "'");
}

void NamedTypeNode::print(TreePrinter &printer) const {
    printer.print_node(this);
}

std::string NamedTypeNode::type_string() const {
    return token().data();
}

PointerTypeNode::PointerTypeNode()
        : TypeNode(Token::synthetic("<*>")), m_pointed_type(),
        m_pointed_type_internal(&Any) {}

PointerTypeNode::PointerTypeNode(Token token, 
        std::unique_ptr<TypeNode> pointed_type)
        : TypeNode(token), m_pointed_type(std::move(pointed_type)), 
        m_pointed_type_internal(m_pointed_type.get()) {}

void PointerTypeNode::set_internal(TypeNode *pointed_type_internal) {
    m_pointed_type_internal = pointed_type_internal;
}

void PointerTypeNode::resolve_locals(SymbolTable &symbol_table, 
        ScopeTracker &scopes) {
    m_pointed_type_internal->resolve_locals(symbol_table, scopes);
}

TypeMatch PointerTypeNode::matching(TypeNode const *node) const {
    PointerTypeNode const *other = dynamic_cast<PointerTypeNode const *>(node);
    if (other == nullptr) {
        ArrayTypeNode const *other = dynamic_cast<ArrayTypeNode const *>(node);
        if (other == nullptr) {
            return TypeMatch::NoMatch;
        }
        TypeMatch match = m_pointed_type_internal
                ->matching(other->pointed_type());
        return weakest_match(TypeMatch::ImplicitMatch, match);
    }
    return m_pointed_type_internal
            ->matching(other->pointed_type());
}

TypeNode const *PointerTypeNode::called_type() const {
    throw std::runtime_error("Cannot call pointer type");
}

TypeNode const *PointerTypeNode::pointed_type() const {
    return m_pointed_type_internal;
}

void PointerTypeNode::print(TreePrinter &printer) const {
    printer.print_node(this);
    printer.last_child(m_pointed_type_internal);
}

std::string PointerTypeNode::type_string() const {
    return m_pointed_type_internal->type_string() + "*";
}

ArrayTypeNode::ArrayTypeNode(Token token, std::unique_ptr<TypeNode> array_type,
        std::unique_ptr<ExpressionNode> size) 
        : TypeNode(token), m_array_type(std::move(array_type)), 
        m_size(std::move(size)) {}

void ArrayTypeNode::resolve_locals(SymbolTable &symbol_table, 
        ScopeTracker &scopes) {
    m_array_type->resolve_locals(symbol_table, scopes);
    m_size->resolve_locals(symbol_table, scopes); // todo extend resolve_types to m_type
}

TypeMatch ArrayTypeNode::matching(TypeNode const *node) const {
    ArrayTypeNode const *other = dynamic_cast<ArrayTypeNode const *>(node);
    if (other == nullptr) {
        PointerTypeNode const *other = 
                dynamic_cast<PointerTypeNode const *>(node);
        if (other == nullptr) {
            return TypeMatch::NoMatch;
        }
        TypeMatch match = m_array_type->matching(other->pointed_type());
        return weakest_match(TypeMatch::ImplicitMatch, match);
    }
    return m_array_type->matching(other->pointed_type());
}

TypeNode const *ArrayTypeNode::called_type() const {
    throw std::runtime_error("Cannot call array type");
}

TypeNode const *ArrayTypeNode::pointed_type() const {
    return m_array_type.get();
}

void ArrayTypeNode::print(TreePrinter &printer) const {
    printer.print_node(this);
    printer.next_child(m_array_type.get());
    printer.last_child(m_size.get());
}

std::string ArrayTypeNode::type_string() const {
    return m_array_type->type_string() + "[" + "(N)" + "]";
}

TypeListNode::TypeListNode(std::vector<std::unique_ptr<TypeNode>> type_list)
        : TypeNode(Token::synthetic("<type-list>")), 
        m_type_list(std::move(type_list)) {}

void TypeListNode::resolve_locals(SymbolTable &symbol_table, 
        ScopeTracker &scopes) {
    for (auto const &entry : m_type_list) {
        entry->resolve_locals(symbol_table, scopes);
    }
}

TypeMatch TypeListNode::matching(TypeNode const *node) const {
    TypeListNode const *other = dynamic_cast<TypeListNode const *>(node);
    if (other == nullptr) {
        throw std::runtime_error(
                "cannot match types: " + type_string() 
                + " and " + node->type_string());
    }
    if (list().size() != other->list().size()) {
        return TypeMatch::NoMatch;
    }
    TypeMatch match = TypeMatch::ExactMatch;
    for (std::size_t i = 0; i < list().size(); i++) {
        match = weakest_match(match, 
                list()[i]->matching(other->list()[i].get()));
    }
    return match;
}

TypeNode const *TypeListNode::called_type() const {
    throw std::runtime_error("Cannot call typelist");
}

TypeNode const *TypeListNode::pointed_type() const {
    throw std::runtime_error("No pointed type for typelist");
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

void CallableTypeNode::resolve_locals(SymbolTable &symbol_table, 
        ScopeTracker &scopes) {
    m_param_types->resolve_locals(symbol_table, scopes);
    m_return_type->resolve_locals(symbol_table, scopes);
}

TypeMatch CallableTypeNode::matching(TypeNode const *node) const {
    if (dynamic_cast<AnyTypeNode const *>(node) != nullptr) {
        return TypeMatch::AnyMatch;
    }
    CallableTypeNode const *other = dynamic_cast<CallableTypeNode const *>(node);
    return weakest_match(
            m_param_types->matching(other->param_types()), 
            m_return_type->matching(other->return_type()));
}

TypeNode const *CallableTypeNode::called_type() const {
    return m_return_type.get();
}

TypeNode const *CallableTypeNode::pointed_type() const {
    throw std::runtime_error("No pointed type for callable type");
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

void ExpressionNode::resolve_globals(SymbolTable &, SymbolMap &) {}

TypeNode *ExpressionNode::type() const {
    return m_type;
}

StatementNode::StatementNode(Token token)
        : BaseNode(token) {}

VariableNode::VariableNode(Token token)
        : ExpressionNode(token) {}

bool VariableNode::is_lvalue() const {
    return true;
}

void VariableNode::resolve_locals(SymbolTable &, ScopeTracker &scopes) {
    set_id(lookup_symbol(token().data(), scopes));
}

void VariableNode::resolve_types(SymbolTable &symbol_table) {
    m_type = symbol_table.get(id()).type;
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
            serializer.inline_frames().use(serializer, entry.id);
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
            serializer.inline_frames().use_address(serializer, entry.id);
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

void LiteralNode::resolve_locals(SymbolTable &, ScopeTracker &) {}

void LiteralNode::resolve_types(SymbolTable &) {}

void LiteralNode::print(TreePrinter &printer) const {
    printer.print_node(this);
}

IntegerLiteralNode::IntegerLiteralNode(Token token, TypeNode *type)
        : LiteralNode(token, type), m_value(token.to_int()) {
}

void IntegerLiteralNode::serialize(Serializer &serializer) const {
    serializer.add_instr(OpCode::Push, m_value);
}

TrueLiteralNode::TrueLiteralNode(Token token, TypeNode *type)
        : LiteralNode(token, type) {
}

void TrueLiteralNode::serialize(Serializer &serializer) const {
    serializer.add_instr(OpCode::Push, 1);
}

FalseLiteralNode::FalseLiteralNode(Token token, TypeNode *type)
        : LiteralNode(token, type) {
}

void FalseLiteralNode::serialize(Serializer &serializer) const {
    serializer.add_instr(OpCode::Push, false);
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

void UnaryExpressionNode::resolve_locals(SymbolTable &symbol_table, 
        ScopeTracker &scopes) {
    m_operand->resolve_locals(symbol_table, scopes);
}

AddressOfNode::AddressOfNode(Token token, 
        std::unique_ptr<ExpressionNode> operand)
        : UnaryExpressionNode(token, std::move(operand)) {}

void AddressOfNode::resolve_types(SymbolTable &symbol_table) {
    m_operand->resolve_types(symbol_table);
    m_pointer_type.set_internal(m_operand->type());
    m_type = &m_pointer_type;
}

void AddressOfNode::serialize(Serializer &serializer) const {
    m_operand->serialize_load_address(serializer);
}

DereferenceNode::DereferenceNode(Token token, 
        std::unique_ptr<ExpressionNode> operand)
        : UnaryExpressionNode(token, std::move(operand)) {}

bool DereferenceNode::is_lvalue() const {
    return true;
}

void DereferenceNode::resolve_types(SymbolTable &symbol_table) {
    m_operand->resolve_types(symbol_table);
    m_type = const_cast<TypeNode *>(m_operand->type()->pointed_type());
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

void BinaryExpressionNode::resolve_locals(SymbolTable &symbol_table, 
        ScopeTracker &scopes) {
    m_left->resolve_locals(symbol_table, scopes);
    m_right->resolve_locals(symbol_table, scopes);
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

void AssignNode::resolve_types(SymbolTable &symbol_table) {
    m_left->resolve_types(symbol_table);
    m_right->resolve_types(symbol_table);
    m_type = m_left->type();
}

void AssignNode::serialize(Serializer &serializer) const {
    m_left->serialize_load_address(serializer);
    m_right->serialize(serializer);
    serializer.add_instr(OpCode::Binary, FuncCode::Assign);
}

AndNode::AndNode(Token token, std::unique_ptr<ExpressionNode> left, 
        std::unique_ptr<ExpressionNode> right, TypeNode *type)
        : BinaryExpressionNode(token, std::move(left), std::move(right)) {
    m_type = type;
}

void AndNode::resolve_types(SymbolTable &symbol_table) {
    m_left->resolve_types(symbol_table);
    m_right->resolve_types(symbol_table);
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

OrNode::OrNode(Token token, 
        std::unique_ptr<ExpressionNode> left, 
        std::unique_ptr<ExpressionNode> right,
        TypeNode *type)
        : BinaryExpressionNode(token, std::move(left), std::move(right)) {
    m_type = type;
}

void OrNode::resolve_types(SymbolTable &symbol_table) {
    m_left->resolve_types(symbol_table);
    m_right->resolve_types(symbol_table);
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

SubscriptNode::SubscriptNode(
        std::unique_ptr<ExpressionNode> left, 
        std::unique_ptr<ExpressionNode> right)
        : BinaryExpressionNode(Token::synthetic("<subscript>"), 
        std::move(left), std::move(right)) {}

bool SubscriptNode::is_lvalue() const {
    return true;
}

void SubscriptNode::resolve_types(SymbolTable &symbol_table) {
    m_left->resolve_types(symbol_table);
    m_right->resolve_types(symbol_table);
    m_type = const_cast<TypeNode *>(m_left->type()->pointed_type());
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
        m_args(std::move(args)), m_overload_id(0) {}

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

void CallNode::resolve_locals(SymbolTable &symbol_table, 
        ScopeTracker &scopes) {
    m_func->resolve_locals(symbol_table, scopes);
    m_args->resolve_locals(symbol_table, scopes);
}

void CallNode::resolve_types(SymbolTable &symbol_table) {
    m_func->resolve_types(symbol_table);
    m_args->resolve_types(symbol_table);

    SymbolEntry const &entry = symbol_table.get(m_func->id());
    
    if (entry.storage_type != StorageType::Callable) {
        m_type = const_cast<TypeNode *>(m_func->type()->called_type());
        return;
    }

    SymbolId candidates[3] = {};
    bool multiple[3] = {};

    for (auto const &id : symbol_table.callable(entry.id)) {
        CallableNode *node = dynamic_cast<CallableNode *>(symbol_table.get(id).definition);
        TypeMatch match = node->is_matching_call(m_args->exprs());
        std::size_t index = static_cast<std::size_t>(match);
        if (candidates[index]) {
            multiple[index] = true;
        }
        if (match != TypeMatch::NoMatch) {
            candidates[index] = id; 
        }
    }
    
    for (std::size_t i = 3; i > 1; i--) {
        std::size_t index = i - 1;
        if (candidates[index]) {
            if (multiple[index]) {
                throw std::runtime_error("Multiple candidates for call");
            }
            m_overload_id = candidates[index];
        }
    }

    if (m_overload_id == 0) {
        throw std::runtime_error("No matching call found");
    }

    m_type = const_cast<TypeNode *>(
            symbol_table.get(m_overload_id).type->called_type());
}

void CallNode::serialize(Serializer &serializer) const {
    SymbolEntry const &entry = serializer.symbol_table().get(m_func->id());
    IntrinsicEntry intrinsic;
    switch (entry.storage_type) {
        case StorageType::Callable:
            serializer.call(m_overload_id, m_args->exprs());
            break;
        case StorageType::Intrinsic:
            intrinsic = intrinsics[entry.value];
            if (m_args->exprs().size() != intrinsic.n_args) {
                throw std::runtime_error(
                        "Invalid intrinsic invocation of " + intrinsic.symbol);
            }
            m_args->serialize(serializer);
            serializer.add_instr(intrinsic.opcode, intrinsic.funccode);
            break;
        default:
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

void TernaryNode::resolve_locals(SymbolTable &symbol_table, 
        ScopeTracker &scopes) {
    m_cond->resolve_locals(symbol_table, scopes);
    m_case_true->resolve_locals(symbol_table, scopes);
    m_case_false->resolve_locals(symbol_table, scopes);
}

void TernaryNode::resolve_types(SymbolTable &symbol_table) {
    m_cond->resolve_types(symbol_table);
    m_case_true->resolve_types(symbol_table);
    m_case_false->resolve_types(symbol_table);
    if (m_case_true->type()->matching(m_case_false->type()) != TypeMatch::ExactMatch) {
        throw std::runtime_error("Types in ternary do not match");
    }
    m_type = m_case_true->type();
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

AttributeNode::AttributeNode(Token token, 
        std::unique_ptr<ExpressionNode> object,
        std::unique_ptr<VariableNode> attribute)
        : ExpressionNode(token), m_object(std::move(object)), 
        m_attribute(std::move(attribute)) {}

void AttributeNode::resolve_locals(SymbolTable &symbol_table, 
        ScopeTracker &scopes) {
    m_object->resolve_locals(symbol_table, scopes);
}

void AttributeNode::resolve_types(SymbolTable &symbol_table) {
    m_object->resolve_types(symbol_table);
    m_type = &Any;
}

void AttributeNode::serialize(Serializer &serializer) const {
    m_object->serialize(serializer);
    serializer.add_instr(OpCode::Pop);
    serializer.add_instr(OpCode::Push, 42);
}

void AttributeNode::print(TreePrinter &printer) const {
    printer.print_node(this);
    printer.next_child(m_object.get());
    printer.last_child(m_attribute.get());
}

LambdaNode::LambdaNode(Token token, 
        CallableSignature signature, 
        std::unique_ptr<StatementNode> body)
        : ExpressionNode(token), m_body(std::move(body)),
        m_signature(std::move(signature)) {}

void LambdaNode::resolve_locals(SymbolTable &symbol_table, ScopeTracker &scopes) {
    uint32_t position = -3 - m_signature.params.size();

    for (std::size_t i = 0; i < m_signature.params.size(); i++) {
        Token const &token = m_signature.params[i];
        TypeNode *type = m_signature.type->param_types()->list()[i].get();

        symbol_table.declare(scopes.current, token.data(), 
                this, type, StorageType::Relative, position);

        position++;
    }
    
    m_body->resolve_locals(symbol_table, scopes);
}

void LambdaNode::resolve_types(SymbolTable &symbol_table) {
    m_body->resolve_types(symbol_table);
    m_type = m_signature.type.get();
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

void CallableNode::resolve_types(SymbolTable &symbol_table) {
    m_body->resolve_types(symbol_table);
}

TypeMatch CallableNode::is_matching_call(
        std::vector<std::unique_ptr<ExpressionNode>> const &args) const {
    if (args.size() != n_params()) {
        return TypeMatch::NoMatch;
    }
    TypeMatch match = TypeMatch::ExactMatch;
    for (std::size_t i = 0; i < args.size(); i++) {
        auto const &param_type = m_signature.type->param_types()->list()[i];
        match = weakest_match(match, param_type->matching(args[i]->type()));
    }
    return match;
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
        CallableSignature signature, std::unique_ptr<BaseNode> body,
        bool writeback)
        : CallableNode(token, ident, std::move(signature), std::move(body)),
        m_writeback(writeback) {}

void FunctionNode::resolve_globals(
        SymbolTable &symbol_table, SymbolMap &symbol_map) {
    set_id(symbol_table.declare_callable(ident().data(), 
            symbol_map, this));
}

void FunctionNode::resolve_locals(SymbolTable &symbol_table, 
        ScopeTracker &scopes) {
    // todo replace by block: -> {{ fn (...) {...} }}
    uint32_t position = -3 - n_params();

    for (std::size_t i = 0; i < n_params(); i++) {
        Token const &token = params()[i];
        
        TypeNode *type = signature().type->param_types()->list()[i].get();
        symbol_table.declare(scopes.current, token.data(), 
                this, type, StorageType::Relative, position);
        position++;
    }

    symbol_table.open_container();
    m_body->resolve_locals(symbol_table, scopes);
    m_frame_size = symbol_table.container_size();
    symbol_table.resolve_local_container();
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
        if (m_writeback && &node == &args.front()) {
            node->serialize_load_address(serializer);
            serializer.add_instr(OpCode::DupLoad);
        } else {
            node->serialize(serializer);
        }
    }
    serializer.add_instr(OpCode::Push, n_params());
    serializer.add_instr(OpCode::Push, id(), true);
    serializer.add_instr(OpCode::Call);
    if (m_writeback) {
        serializer.add_instr(OpCode::Binary, FuncCode::Assign);
    }
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
        CallableSignature signature, std::unique_ptr<BaseNode> body, 
        bool writeback)
        : CallableNode(token, ident, std::move(signature), std::move(body)), 
        m_param_ids(), m_writeback(writeback) {}

void InlineNode::resolve_globals(
        SymbolTable &symbol_table, SymbolMap &symbol_map) {
    set_id(symbol_table.declare_callable(ident().data(), 
            symbol_map, this));
}

void InlineNode::resolve_locals(SymbolTable &symbol_table, ScopeTracker &scopes) {
    uint32_t position = 0;

    for (std::size_t i = 0; i < n_params(); i++) {
        Token const &token = params()[i];
        TypeNode *type = signature().type->param_types()->list()[i].get();

        SymbolId id = symbol_table.declare(scopes.current, 
                token.data(), this, type, StorageType::InlineReference, 
                position);
        m_param_ids.push_back(id);

        position++;
    }

    m_body->resolve_locals(symbol_table, scopes);
}

void InlineNode::serialize(Serializer &) const {}

void InlineNode::serialize_call(Serializer &serializer, 
        std::vector<std::unique_ptr<ExpressionNode>> const &args) const {
    serializer.inline_frames().open_call(args, m_param_ids, m_writeback);
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

void EmptyNode::resolve_globals(SymbolTable &, SymbolMap &) {}

void EmptyNode::resolve_locals(SymbolTable &, ScopeTracker &) {}

void EmptyNode::resolve_types(SymbolTable &) {}

void EmptyNode::serialize(Serializer &) const {}

void EmptyNode::print(TreePrinter &printer) const {
    printer.print_node(this);
}

BlockNode::BlockNode(std::vector<std::unique_ptr<StatementNode>> statements)
        : StatementNode(Token::synthetic("<block>")), 
        m_statements(std::move(statements)), m_scope_map() {}

void BlockNode::resolve_globals(
        SymbolTable &symbol_table, SymbolMap &symbol_map) {
    for (auto const &stmt : m_statements) {
        stmt->resolve_globals(symbol_table, symbol_map);
    }
}

void BlockNode::resolve_locals(SymbolTable &symbol_table, 
        ScopeTracker &scopes) {
    for (auto const &stmt : m_statements) {
        stmt->resolve_locals(symbol_table, scopes);
    }
}

void BlockNode::serialize(Serializer &serializer) const {
    for (std::unique_ptr<StatementNode> const &stmt : m_statements) {
        stmt->serialize(serializer);
    }
}

void BlockNode::resolve_types(SymbolTable &symbol_table) {
    for (auto const &stmt : m_statements) {
        stmt->resolve_types(symbol_table);
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

ScopeNode::ScopeNode(std::unique_ptr<StatementNode> statement)
        : StatementNode(Token::synthetic("<scope>")), 
        m_statement(std::move(statement)) {}

void ScopeNode::resolve_globals(SymbolTable &symbol_table, 
        SymbolMap &symbol_map) {
    m_statement->resolve_globals(symbol_table, symbol_map);
    symbol_table.add_job(this);
}

void ScopeNode::resolve_locals(SymbolTable &symbol_table, 
        ScopeTracker &scopes) {
    ScopeTracker block_scopes(scopes.global, scopes.enclosing, {});
    for (auto const &entry : scopes.current) {
        block_scopes.enclosing[entry.first] = entry.second;
    }
    m_statement->resolve_locals(symbol_table, block_scopes);
}

void ScopeNode::resolve_types(SymbolTable &symbol_table) {
    m_statement->resolve_types(symbol_table);
}

void ScopeNode::serialize(Serializer &serializer) const {
    m_statement->serialize(serializer);
}

void ScopeNode::print(TreePrinter &printer) const {
    printer.print_node(this);
    printer.last_child(m_statement.get());
}

TypeDeclarationNode::TypeDeclarationNode(
        Token token, std::unique_ptr<NamedTypeNode> ident)
        : StatementNode(token), m_ident(std::move(ident)) {}

void TypeDeclarationNode::resolve_globals(
        SymbolTable &symbol_table, SymbolMap &symbol_map) {
    set_id(symbol_table.declare(symbol_map, 
            m_ident->token().data(), this, nullptr, StorageType::Type));
}

void TypeDeclarationNode::resolve_locals(SymbolTable &, ScopeTracker &) {}

void TypeDeclarationNode::resolve_types(SymbolTable &) {}

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

void ExpressionListNode::resolve_globals(SymbolTable &, SymbolMap &) {}

void ExpressionListNode::resolve_locals(SymbolTable &symbol_table, 
        ScopeTracker &scopes) {
    for (auto const &expr : m_exprs) {
        expr->resolve_locals(symbol_table, scopes);
    }
}

void ExpressionListNode::resolve_types(SymbolTable &symbol_table) {
    for (auto const &expr : m_exprs) {
        expr->resolve_types(symbol_table);
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

void IfNode::resolve_globals(SymbolTable &, SymbolMap &) {}

void IfNode::resolve_locals(SymbolTable &symbol_table, 
        ScopeTracker &scopes) {
    m_cond->resolve_locals(symbol_table, scopes);
    m_case_true->resolve_locals(symbol_table, scopes);
}

void IfNode::resolve_types(SymbolTable &symbol_table) {
    m_cond->resolve_types(symbol_table);
    m_case_true->resolve_types(symbol_table);
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

void IfElseNode::resolve_globals(SymbolTable &, SymbolMap &) {}

void IfElseNode::resolve_locals(SymbolTable &symbol_table, 
        ScopeTracker &scopes) {
    m_cond->resolve_locals(symbol_table, scopes);
    m_case_true->resolve_locals(symbol_table, scopes);
    m_case_false->resolve_locals(symbol_table, scopes);
}

void IfElseNode::resolve_types(SymbolTable &symbol_table) {
    m_cond->resolve_types(symbol_table);
    m_case_true->resolve_types(symbol_table);
    m_case_false->resolve_types(symbol_table);
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

void ForLoopNode::resolve_globals(SymbolTable &, SymbolMap &) {}

void ForLoopNode::resolve_locals(SymbolTable &symbol_table, 
        ScopeTracker &scopes) {
    m_init->resolve_locals(symbol_table, scopes);
    m_cond->resolve_locals(symbol_table, scopes);
    m_post->resolve_locals(symbol_table, scopes);
    m_body->resolve_locals(symbol_table, scopes);
}

void ForLoopNode::resolve_types(SymbolTable &symbol_table) {
    m_init->resolve_types(symbol_table);
    m_cond->resolve_types(symbol_table);
    m_post->resolve_types(symbol_table);
    m_body->resolve_types(symbol_table);
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

void ReturnNode::resolve_globals(SymbolTable &, SymbolMap &) {}

void ReturnNode::resolve_locals(SymbolTable &symbol_table, 
        ScopeTracker &scopes) {
    m_operand->resolve_locals(symbol_table, scopes);
}

void ReturnNode::resolve_types(SymbolTable &symbol_table) {
    m_operand->resolve_types(symbol_table);
}

void ReturnNode::serialize(Serializer &serializer) const {
    // todo: first push ret dest addr, then do store, then ret wo val
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
        SymbolTable &symbol_table, SymbolMap &current) {
    if (m_init_value != nullptr) {
        throw std::runtime_error("not implemented");
    }
    set_id(symbol_table.declare(current, m_ident.data(), 
            this, m_type.get(), 
            m_size == nullptr ? 
                StorageType::Absolute : StorageType::AbsoluteRef, 
            0, declared_size()));
    symbol_table.add_to_container(id());
}

void VarDeclarationNode::resolve_locals(SymbolTable &symbol_table, 
        ScopeTracker &scopes) {
    if (m_type != nullptr) {
        m_type->resolve_locals(symbol_table, scopes);
    }
    if (m_init_value != nullptr) {
        m_init_value->resolve_locals(symbol_table, scopes);
    }
    set_id(symbol_table.declare(scopes.current, m_ident.data(), 
            this, m_type.get(), 
            m_size == nullptr ? 
                StorageType::Relative : StorageType::RelativeRef, 
            0, declared_size()));
    symbol_table.add_to_container(id());
}

void VarDeclarationNode::resolve_types(SymbolTable &symbol_table) {
    if (m_size != nullptr) {
        m_size->resolve_types(symbol_table);
    }
    if (m_init_value != nullptr) {
        m_init_value->resolve_types(symbol_table);
        TypeMatch match = m_type->matching(m_init_value->type());
        if (match == TypeMatch::NoMatch) {
            throw std::runtime_error("Type mismatch: " + to_string(token()));
        }
    }
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

void ExpressionStatementNode::resolve_globals(SymbolTable &, SymbolMap &) {}

void ExpressionStatementNode::resolve_locals(SymbolTable &symbol_table, 
        ScopeTracker &scopes) {
    m_expr->resolve_locals(symbol_table, scopes);
}

void ExpressionStatementNode::resolve_types(SymbolTable &symbol_table) {
    m_expr->resolve_types(symbol_table);
}

void ExpressionStatementNode::serialize(Serializer &serializer) const {
    m_expr->serialize(serializer);
    serializer.add_instr(OpCode::Pop);
}

void ExpressionStatementNode::print(TreePrinter &printer) const {
    printer.print_node(this);
    printer.last_child(m_expr.get());
}
