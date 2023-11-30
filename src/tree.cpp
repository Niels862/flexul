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
        Serializer &serializer, SymbolMap &global_scope, 
        SymbolMap &enclosing_scope, SymbolMap &current_scope) {
    for (BaseNode *child : get_children()) {
        child->resolve_symbols_second_pass(
                serializer, global_scope, 
                enclosing_scope, current_scope);
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

BaseNode *BaseNode::get_fourth() const {
    return get_nth(3, "fourth");
}

void BaseNode::set_id(SymbolId id) {
    symbol_id = id;
}

SymbolId BaseNode::get_id() const {
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
        std::cerr << labelPrefix << "(null)" << std::endl;
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
                branchPrefix + "\u251C\u2500", 
                branchPrefix + "\u2502 ");
    }
    BaseNode::print(node->children[i], 
            branchPrefix + "\u2570\u2500", 
            branchPrefix + "  ");
}

EmptyNode::EmptyNode(Token token, std::vector<BaseNode *> children)
        : BaseNode(0, token, children) {}

void EmptyNode::serialize(Serializer &) const {}

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
        Serializer &, SymbolMap &global_scope, 
        SymbolMap &enclosing_scope, SymbolMap &current_scope) {
    SymbolId id = lookup_symbol(get_token().get_data(), global_scope, 
            enclosing_scope, current_scope);
    set_id(id);
}

uint32_t VariableNode::register_symbol(
        Serializer &serializer, SymbolMap &scope, 
        StorageType storage_type, uint32_t value) const {
    SymbolId symbol_id = serializer.get_symbol_id();
    declare_symbol(get_token().get_data(), symbol_id, scope);
    serializer.register_symbol({
        get_token().get_data(), 
        symbol_id, 
        storage_type, 
        value,
        0
    });
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
        throw std::runtime_error("Cannot load address of address");
    } else if (entry.storage_type == StorageType::Relative) {
        serializer.add_instr(OpCode::LoadAddrRel, entry.value);
    } else {
        throw std::runtime_error(
                "Invalid storage type: " 
                + std::to_string(static_cast<int>(entry.storage_type)));
    }
}

UnaryNode::UnaryNode(Token token, std::vector<BaseNode *> children)
        : BaseNode(1, token, children) {}

bool UnaryNode::is_lvalue() const {
    return get_token().get_data() == "*";
}

void UnaryNode::serialize(Serializer &serializer) const {
    Token token = get_token();
    if (token.get_data() == "&") {
        get_first()->serialize_load_address(serializer);
        return;
    }
    get_first()->serialize(serializer);
    if (token.get_data() == "-") {
        serializer.add_instr(OpCode::Unary, FuncCode::Neg);
    } else if (token.get_data() == "*") {
        serializer.add_instr(OpCode::LoadAbs);
    } else if (token.get_data() != "*") {
        throw std::runtime_error(
                "Unrecognized operator for unary expression: " 
                + token.get_data());
    }
}

void UnaryNode::serialize_load_address(Serializer &serializer) const {
    if (get_token().get_data() != "*") {
        throw std::runtime_error("Cannot load address of non lvalue");
    }
    get_first()->serialize(serializer);
}

BinaryNode::BinaryNode(Token token, std::vector<BaseNode *> children)
        : BaseNode(2, token, children) {}

void BinaryNode::serialize(Serializer &serializer) const {
    FuncCode funccode;
    Token token = get_token();
    Label label_true, label_false, label_end;

    if (token.get_data() == "=") {
        get_first()->serialize_load_address(serializer);
        get_second()->serialize(serializer);
        serializer.add_instr(OpCode::Binary, FuncCode::Assign);
        return;
    }

    if (token.get_data() == ">" || token.get_data() == ">=") {
        if (token.get_data() == ">=") {
            funccode = FuncCode::LessEquals;
        } else {
            funccode = FuncCode::LessThan;
        }
        get_second()->serialize(serializer);
        get_first()->serialize(serializer);
        serializer.add_instr(OpCode::Binary, funccode);
        return;
    }

    if (token.get_data() == "||") {
        label_true = serializer.get_label();
        label_end = serializer.get_label();

        get_first()->serialize(serializer);
        serializer.add_instr(OpCode::BrTrue, label_true, true);

        get_second()->serialize(serializer);
        serializer.add_instr(OpCode::BrTrue, label_true, true);
        serializer.add_instr(OpCode::Push, 0);
        serializer.add_instr(OpCode::Jump, label_end, true);

        serializer.add_label(label_true);
        serializer.add_instr(OpCode::Push, 1);

        serializer.add_label(label_end);
        return;
    }

    if (token.get_data() == "&&") {
        label_false = serializer.get_label();
        label_end = serializer.get_label();

        get_first()->serialize(serializer);
        serializer.add_instr(OpCode::BrFalse, label_false, true);

        get_second()->serialize(serializer);
        serializer.add_instr(OpCode::BrFalse, label_false, true);
        serializer.add_instr(OpCode::Push, 1);
        serializer.add_instr(OpCode::Jump, label_end, true);

        serializer.add_label(label_false);
        serializer.add_instr(OpCode::Push, 0);

        serializer.add_label(label_end);
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
    } else if (token.get_data() == "==") {
        funccode = FuncCode::Equals;
    } else if (token.get_data() == "!=") {
        funccode = FuncCode::NotEquals;
    } else if (token.get_data() == "<") {
        funccode = FuncCode::LessThan;
    } else if (token.get_data() == "<=") {
        funccode = FuncCode::LessEquals;
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
    Label label_false = serializer.get_label();
    Label label_end = serializer.get_label();
    
    get_first()->serialize(serializer);
    serializer.add_instr(OpCode::BrFalse, label_false, true);

    get_second()->serialize(serializer);
    serializer.add_instr(OpCode::Jump, label_end, true);

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
        : BaseNode(token, children), scope_map() {}

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

ScopedBlockNode::ScopedBlockNode(Token token, std::vector<BaseNode *> children)
        : BaseNode(token, children), scope_map() {}

void ScopedBlockNode::serialize(Serializer &serializer) const {
    for (BaseNode const *child : get_children()) {
        child->serialize(serializer);
    }
}

FunctionNode::FunctionNode(Token token, std::vector<BaseNode *> children)
        : BaseNode(3, token, children) {}

void FunctionNode::resolve_symbols_first_pass(
        Serializer &serializer, SymbolMap &symbol_map) {
    SymbolId func_id = get_first()->register_symbol(
            serializer, symbol_map, StorageType::Absolute, 0);
    set_id(func_id);
}

void FunctionNode::resolve_symbols_second_pass(
        Serializer &serializer, SymbolMap &global_scope, 
        SymbolMap &, SymbolMap &) {
    // used as enclosing scope which is empty
    // for function
    SymbolMap empty_scope;
    SymbolMap function_scope;
    uint32_t position = -3 - get_second()->get_children().size();
    for (BaseNode *child : get_second()->get_children()) {
        child->set_id(child->register_symbol(
                serializer, function_scope, 
                StorageType::Relative, position));
        position++;
    }
    serializer.open_container();
    get_third()->resolve_symbols_second_pass(serializer, global_scope, 
            empty_scope, function_scope);
    frame_size = serializer.resolve_container();
}

void FunctionNode::serialize(Serializer &serializer) const {
    SymbolId id = get_id();
    if (id == 0) {
        throw std::runtime_error("Unresolved name");
    }
    serializer.add_label(id);
    serializer.add_instr(OpCode::AddSp, frame_size);
    get_third()->serialize(serializer);
    serializer.add_instr(OpCode::Ret, 0);
}

LambdaNode::LambdaNode(
        Token token, std::vector<BaseNode *> children)
        : BaseNode(2, token, children) {}

void LambdaNode::resolve_symbols_second_pass(
        Serializer &serializer, SymbolMap &global_scope, 
        SymbolMap &, SymbolMap &) {
    SymbolMap empty_scope;
    SymbolMap lambda_scope;
    uint32_t position = -3 - get_first()->get_children().size();
    for (BaseNode *child : get_first()->get_children()) {
        child->set_id(child->register_symbol(
                serializer, lambda_scope, 
                StorageType::Relative, position));
        position++;
    }
    get_second()->resolve_symbols_second_pass(serializer, global_scope, 
            empty_scope, lambda_scope);
}

void LambdaNode::serialize(Serializer &serializer) const {
    SymbolId id = serializer.get_label();
    serializer.add_job(id, get_second());
    serializer.add_instr(OpCode::Push, id, true);
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
    Label label_end = serializer.get_label();
    get_first()->serialize(serializer);
    serializer.add_instr(OpCode::BrFalse, label_end, true);
    get_second()->serialize(serializer);
    serializer.add_label(label_end);
}

ForLoopNode::ForLoopNode(Token token, std::vector<BaseNode *> children)
        : BaseNode(4, token, children) {}

void ForLoopNode::serialize(Serializer &serializer) const {
    Label loop_body_label = serializer.get_label();
    Label cond_label = serializer.get_label();

    get_first()->serialize(serializer); // init: statement
    serializer.add_instr(OpCode::Jump, cond_label, true);
    
    serializer.add_label(loop_body_label); // body and post: statements
    get_fourth()->serialize(serializer);
    get_third()->serialize(serializer);

    serializer.add_label(cond_label);
    get_second()->serialize(serializer); // cond: expression
    serializer.add_instr(OpCode::BrTrue, loop_body_label, true);
}

ReturnNode::ReturnNode(Token token, std::vector<BaseNode *> children)
        : BaseNode(1, token, children) {}

void ReturnNode::serialize(Serializer &serializer) const {
    get_first()->serialize(serializer);
    serializer.add_instr(OpCode::Ret);
}

DeclarationNode::DeclarationNode(Token token, std::vector<BaseNode *> children)
        : BaseNode(2, token, children) {}

void DeclarationNode::resolve_symbols_second_pass(
        Serializer &serializer, SymbolMap &global_scope, 
        SymbolMap &enclosing_scope, SymbolMap &current_scope) {
    if (get_second() != nullptr) {
        get_second()->resolve_symbols_second_pass(serializer, global_scope, 
                enclosing_scope, current_scope);
    }
    serializer.add_to_container(get_first()->register_symbol(
            serializer, current_scope, StorageType::Relative, 0));
    get_first()->resolve_symbols_second_pass(serializer, global_scope, 
            enclosing_scope, current_scope);
}

void DeclarationNode::serialize(Serializer &serializer) const {
    if (get_second() != nullptr) {
        get_first()->serialize_load_address(serializer);
        get_second()->serialize(serializer);
        serializer.add_instr(OpCode::Binary, FuncCode::Assign);
        serializer.add_instr(OpCode::Pop);
    }
}

ExpressionStatementNode::ExpressionStatementNode(
        Token token, std::vector<BaseNode *> children)
        : BaseNode(1, token, children) {}

void ExpressionStatementNode::serialize(Serializer &serializer) const {
    get_first()->serialize(serializer);
    serializer.add_instr(OpCode::Pop);
}