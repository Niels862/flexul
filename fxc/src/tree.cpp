#include "tree.hpp"
#include <iostream>

BaseNode::BaseNode(uint32_t arity, Token token, std::vector<BaseNode *> children)
        : token(token), children(children), symbol_id(0) {
    if (arity != children.size()) {
        throw std::runtime_error(
                "Syntax tree arity mismatch: expected " + std::to_string(arity) 
                + ", got " + std::to_string(children.size()));
    }
}

BaseNode::BaseNode(Token token, std::vector<BaseNode *> children)
        : token(token), children(children), symbol_id(0) {}

BaseNode::~BaseNode() {
    for (BaseNode const *child : children) {
        delete child;
    }
}

bool BaseNode::is_lvalue() const {
    return false;
}

void BaseNode::resolve_symbols_first_pass(Serializer &, SymbolMap &) {}

void BaseNode::resolve_symbols_second_pass(
        Serializer &serializer, SymbolMap &symbol_map) {
    for (BaseNode *child : get_children()) {
        child->resolve_symbols_second_pass(serializer, symbol_map);
    }
}

uint32_t BaseNode::register_symbol(
        Serializer &, SymbolMap &, StorageType, uint32_t) const {
    return 0;
}

void BaseNode::serialize_load_address(Serializer &) const {}

Token BaseNode::get_token() const {
    return token;
}

const std::vector<BaseNode *> &BaseNode::get_children() const {
    return children;
}

BaseNode *BaseNode::get_first() const {
    return get_nth(0, "first");
}

BaseNode *BaseNode::get_second() const {
    return get_nth(1, "second");
}

BaseNode *BaseNode::get_third() const {
    return get_nth(2, "third");
}

void BaseNode::set_id(uint32_t id) {
    symbol_id = id;
}

uint32_t BaseNode::get_id() const {
    return symbol_id;
}

BaseNode *BaseNode::get_nth(uint32_t n, std::string const &ordinal) const {
    if (children.size() <= n) {
        throw std::runtime_error(
                "Could not access '" + ordinal + "': have " 
                + std::to_string(children.size()) + " children");
    }
    return children[n];
}

void BaseNode::print(BaseNode *node, std::string const labelPrefix, 
        std::string const branchPrefix) {
    size_t i;
    if (node == nullptr) {
        std::cerr << labelPrefix << "NULL" << std::endl;
        return;
    }
    std::cerr << labelPrefix << node->token.get_data();
    if (node->get_id() != 0) {
        std::cerr << " (" << node->get_id() << ")" << std::endl;
    } else {
        std::cerr << std::endl;
    }
    if (node->children.empty()) {
        return;
    }
    for (i = 0; i < node->children.size() - 1; i++) {
        BaseNode::print(node->children[i], 
                branchPrefix + "\u251C\u2500\u2500\u2500", 
                branchPrefix + "\u2502   ");
    }
    BaseNode::print(node->children[i], 
            branchPrefix + "\u2570\u2500\u2500\u2500", 
            branchPrefix + "    ");
}

IntLitNode::IntLitNode(Token token, std::vector<BaseNode *> children)
        : BaseNode(0, token, children), value(0) {
    try {
        value = std::stoi(token.get_data());
    } catch (std::exception const &e) {
        throw std::runtime_error(
                "Could not convert string to int: " + token.get_data());
    }
}

void IntLitNode::serialize(Serializer &serializer) const {
    serializer.add_instr(OpCode::Push, value);
}

VariableNode::VariableNode(Token token, std::vector<BaseNode *> children)
        : BaseNode(0, token, children) {}

bool VariableNode::is_lvalue() const {
    return true;
}

void VariableNode::resolve_symbols_second_pass(
        Serializer &, SymbolMap &symbol_map) {
    auto iter = symbol_map.find(get_token().get_data());
    if (iter == symbol_map.end()) {
        throw std::runtime_error(
                "Unresolved reference: " + get_token().get_data());
    }
    set_id(iter->second);
}

uint32_t VariableNode::register_symbol(
        Serializer &serializer, SymbolMap &symbol_map, 
        StorageType storage_type, uint32_t value) const {
    uint32_t symbol_id = serializer.get_symbol_id();
    symbol_map[get_token().get_data()] = symbol_id;
    serializer.register_symbol({symbol_id, storage_type, value});
    return symbol_id;
}

void VariableNode::serialize(Serializer &serializer) const {
    SymbolEntry entry = serializer.get_symbol_entry(get_id());
    if (entry.storage_type == StorageType::Absolute) {
        serializer.add_instr(OpCode::Push, get_id(), true);
    } else if (entry.storage_type == StorageType::Relative) {
        serializer.add_instr(OpCode::Push, entry.value);
        serializer.add_instr(OpCode::LoadRel);
    } else {
        throw std::runtime_error(
                "Invalid storage type: " 
                + std::to_string(static_cast<int>(entry.storage_type)));
    }
}

void VariableNode::serialize_load_address(Serializer &serializer) const {
    SymbolEntry entry = serializer.get_symbol_entry(get_id());
    if (entry.storage_type == StorageType::Absolute) { // TODO: think about this
        serializer.add_instr(OpCode::Push, get_id(), true);
    } else if (entry.storage_type == StorageType::Relative) {
        serializer.add_instr(OpCode::Push, entry.value, true);
        serializer.add_instr(OpCode::LoadAddrRel);
    } else {
        throw std::runtime_error(
                "Invalid storage type: " 
                + std::to_string(static_cast<int>(entry.storage_type)));
    }
}

UnaryNode::UnaryNode(Token token, std::vector<BaseNode *> children)
        : BaseNode(1, token, children) {}

void UnaryNode::serialize(Serializer &serializer) const {
    FuncCode funccode;
    Token token = get_token();
    if (token.get_data() == "-") {
        funccode = FuncCode::Neg;
    } else {
        throw std::runtime_error(
                "Unrecognized operator for unary expression: " 
                + token.get_data());
    }
    get_first()->serialize(serializer);
    serializer.add_instr(OpCode::Unary, funccode);
}

BinaryNode::BinaryNode(Token token, std::vector<BaseNode *> children)
        : BaseNode(2, token, children) {}

void BinaryNode::serialize(Serializer &serializer) const {
    FuncCode funccode;
    Token token = get_token();

    if (token.get_data() == "=") {
        get_first()->serialize_load_address(serializer);
        get_second()->serialize(serializer);
        serializer.add_instr(OpCode::Binary, FuncCode::Assign);
        return;
    }

    if (token.get_data() == "+") {
        funccode = FuncCode::Add;
    } else if (token.get_data() == "-") {
        funccode = FuncCode::Sub;
    } else if (token.get_data() == "*") {
        funccode = FuncCode::Mul;
    } else if (token.get_data() == "/") {
        funccode = FuncCode::Div;
    } else if (token.get_data() == "%") {
        funccode = FuncCode::Mod;
    } else {
        throw std::runtime_error(
                "Unrecognized operator for binary expression: "
                + token.get_data());
    }
    get_first()->serialize(serializer);
    get_second()->serialize(serializer);
    serializer.add_instr(OpCode::Binary, funccode);
}

IfElseNode::IfElseNode(Token token, std::vector<BaseNode *> children)
        : BaseNode(3, token, children) {}

void IfElseNode::serialize(Serializer &serializer) const {
    uint32_t label_false = serializer.get_label();
    uint32_t label_end = serializer.get_label();
    get_first()->serialize(serializer);
    serializer.add_instr(OpCode::Push, label_false, true);
    serializer.add_instr(OpCode::BrFalse);
    get_second()->serialize(serializer);
    serializer.add_instr(OpCode::Push, label_end, true);
    serializer.add_instr(OpCode::Jump);
    serializer.add_label(label_false);
    get_third()->serialize(serializer);
    serializer.add_label(label_end);
}

CallNode::CallNode(Token token, std::vector<BaseNode *> children)
        : BaseNode(2, token, children) {}

void CallNode::serialize(Serializer &serializer) const {
    get_second()->serialize(serializer);
    serializer.add_instr(OpCode::Push, get_second()->get_children().size());
    get_first()->serialize(serializer);
    serializer.add_instr(OpCode::Call);
}

BlockNode::BlockNode(Token token, std::vector<BaseNode *> children)
        : BaseNode(token, children) {}

void BlockNode::resolve_symbols_first_pass(
        Serializer &serializer, SymbolMap &symbol_map) {
    for (BaseNode *child : get_children()) {
        child->resolve_symbols_first_pass(serializer, symbol_map);
    }
}

void BlockNode::serialize(Serializer &serializer) const {
    for (BaseNode const *child : get_children()) {
        child->serialize(serializer);
    }
}

FunctionNode::FunctionNode(Token token, std::vector<BaseNode *> children)
        : BaseNode(3, token, children) {}

void FunctionNode::resolve_symbols_first_pass(
        Serializer &serializer, SymbolMap &symbol_map) {
    uint32_t func_id = get_first()->register_symbol(
            serializer, symbol_map, StorageType::Absolute, 0);
    set_id(func_id);
}

void FunctionNode::resolve_symbols_second_pass(
        Serializer &serializer, SymbolMap &symbol_map) {
    SymbolMap function_scope_map = symbol_map;
    uint32_t position = -3 - get_second()->get_children().size();
    for (BaseNode *child : get_second()->get_children()) {
        child->set_id(child->register_symbol(
                serializer, function_scope_map, 
                StorageType::Relative, position));
        position++;
    }
    get_third()->resolve_symbols_second_pass(serializer, function_scope_map);
}

void FunctionNode::serialize(Serializer &serializer) const {
    uint32_t id = get_id();
    if (id == 0) {
        throw std::runtime_error("Unresolved name");
    }
    serializer.add_label(id);
    get_third()->serialize(serializer);
}

ExpressionListNode::ExpressionListNode(
        Token token, std::vector<BaseNode *> children)
        : BaseNode(token, children) {}

void ExpressionListNode::serialize(Serializer &serializer) const {
    for (BaseNode const *child : get_children()) {
        child->serialize(serializer);
    }
}

IfNode::IfNode(
        Token token, std::vector<BaseNode *> children)
        : BaseNode(2, token, children) {}

void IfNode::serialize(Serializer &serializer) const {
    uint32_t label_end = serializer.get_label();
    get_first()->serialize(serializer);
    serializer.add_instr(OpCode::Push, label_end, true);
    serializer.add_instr(OpCode::BrFalse);
    get_second()->serialize(serializer);
}

ReturnNode::ReturnNode(Token token, std::vector<BaseNode *> children)
        : BaseNode(1, token, children) {}

void ReturnNode::serialize(Serializer &serializer) const {
    get_first()->serialize(serializer);
    serializer.add_instr(OpCode::Ret);
}

ExpressionStatementNode::ExpressionStatementNode(
        Token token, std::vector<BaseNode *> children)
        : BaseNode(1, token, children) {}

void ExpressionStatementNode::serialize(Serializer &serializer) const {
    get_first()->serialize(serializer);
    serializer.add_instr(OpCode::Pop);
}
