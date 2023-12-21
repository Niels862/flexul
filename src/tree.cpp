#include "tree.hpp"
#include <iostream>

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
        child->resolve_symbols_second_pass(serializer, global_scope, 
                enclosing_scope, current_scope);
    }
}

void BaseNode::serialize_load_address(Serializer &) const {}

std::string BaseNode::get_label() const {
    return token.get_data();
}

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
    std::cerr << labelPrefix << node->get_label();
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

EmptyNode::EmptyNode()
        : BaseNode(Token::synthetic("<empty>"), {}) {}

void EmptyNode::serialize(Serializer &) const {}

IntLitNode::IntLitNode(Token token)
        : BaseNode(token, {}) {
}

void IntLitNode::serialize(Serializer &serializer) const {
    serializer.add_instr(OpCode::Push, get_token().to_int());
}

VariableNode::VariableNode(Token token)
        : BaseNode(token, {}) {}

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

void VariableNode::serialize(Serializer &serializer) const {
    SymbolEntry entry = serializer.get_symbol_entry(get_id());
    if (entry.storage_type == StorageType::Absolute) {
        serializer.add_instr(OpCode::Push, entry.id, true);
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
        throw std::runtime_error("Cannot load address of address");
    } else if (entry.storage_type == StorageType::Relative) {
        serializer.add_instr(OpCode::LoadAddrRel, entry.value);
    } else {
        throw std::runtime_error(
                "Invalid storage type: " 
                + std::to_string(static_cast<int>(entry.storage_type)));
    }
}

UnaryNode::UnaryNode(Token token, BaseNode *child)
        : BaseNode(token, {child}) {}

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

BinaryNode::BinaryNode(Token token, BaseNode *left, BaseNode *right)
        : BaseNode(token, {left, right}) {}

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

IfElseNode::IfElseNode(Token token, BaseNode *cond, BaseNode *case_true, 
        BaseNode *case_false)
        : BaseNode(token, {cond, case_true, case_false}) {}

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

CallNode::CallNode(BaseNode *func, BaseNode *args)
        : BaseNode(Token::synthetic("<call>"), {func, args}) {}

void CallNode::serialize(Serializer &serializer) const {
    SymbolEntry entry = serializer.get_symbol_entry(get_first()->get_id());
    if (entry.storage_type == StorageType::Intrinsic) {
        IntrinsicEntry intrinsic = intrinsics[entry.value];
        if (get_second()->get_children().size() != intrinsic.n_args) {
            throw std::runtime_error(
                    "Invalid intrinsic invocation of " + intrinsic.symbol);
        }
        get_second()->serialize(serializer);
        serializer.add_instr(intrinsic.opcode, intrinsic.funccode);
    } else {
        get_second()->serialize(serializer);
        serializer.add_instr(OpCode::Push, get_second()->get_children().size());
        get_first()->serialize(serializer);
        serializer.add_instr(OpCode::Call);
    }
}

BlockNode::BlockNode(std::vector<BaseNode *> children)
        : BaseNode(Token::synthetic("<block>"), children), scope_map() {}

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

ScopedBlockNode::ScopedBlockNode(std::vector<BaseNode *> children)
        : BaseNode(Token::synthetic("<scoped-block>"), children), scope_map() {}

void ScopedBlockNode::resolve_symbols_second_pass(
        Serializer &serializer, SymbolMap &global_scope, 
        SymbolMap &enclosing_scope, SymbolMap &current_scope) {
    SymbolMap block_scope;
    SymbolMap block_enclosing_scope = enclosing_scope;
    for (auto const &key_value : current_scope) {
        block_enclosing_scope[key_value.first] = key_value.second;
    }
    for (BaseNode *child : get_children()) {
        child->resolve_symbols_second_pass(serializer, global_scope, 
                block_enclosing_scope, block_scope);
    }
}

void ScopedBlockNode::serialize(Serializer &serializer) const {
    for (BaseNode const *child : get_children()) {
        child->serialize(serializer);
    }
}

FunctionNode::FunctionNode(Token token, Token ident, std::vector<Token> params, 
        BaseNode *body)
        : BaseNode(token, {body}), ident(ident), params(params) {}

void FunctionNode::resolve_symbols_first_pass(
        Serializer &serializer, SymbolMap &symbol_map) {
    set_id(serializer.declare_symbol(ident.get_data(), symbol_map, 
            StorageType::Absolute));
}

void FunctionNode::resolve_symbols_second_pass(
        Serializer &serializer, SymbolMap &global_scope, 
        SymbolMap &, SymbolMap &) {
    // used as enclosing scope which is empty
    // for function
    SymbolMap empty_scope;
    SymbolMap function_scope;
    uint32_t position = -3 - params.size();
    for (Token const &param : params) {
        serializer.declare_symbol(param.get_data(), function_scope, 
                StorageType::Relative, position);
        position++;
    }
    serializer.open_container();
    get_first()->resolve_symbols_second_pass(serializer, global_scope, 
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
    get_first()->serialize(serializer);
    serializer.add_instr(OpCode::Ret, 0);
}

std::string FunctionNode::get_label() const {
    return get_token().get_data() + " " + ident.get_data() + "(" 
            + tokenlist_to_string(params) + ")";
}

LambdaNode::LambdaNode(Token token, std::vector<Token> params, BaseNode *expr)
        : BaseNode(token, {expr}), params(params) {}

void LambdaNode::resolve_symbols_second_pass(
        Serializer &serializer, SymbolMap &global_scope, 
        SymbolMap &, SymbolMap &) {
    SymbolMap empty_scope;
    SymbolMap lambda_scope;
    uint32_t position = -3 - params.size();
    for (Token const &param : params) {
        serializer.declare_symbol(param.get_data(), lambda_scope, 
                StorageType::Relative, position);
        position++;
    }
    get_first()->resolve_symbols_second_pass(serializer, global_scope, 
            empty_scope, lambda_scope);
}

void LambdaNode::serialize(Serializer &serializer) const {
    SymbolId id = serializer.get_label();
    serializer.add_job(id, get_first());
    serializer.add_instr(OpCode::Push, id, true);
}

std::string LambdaNode::get_label() const {
    return get_token().get_data() + " (" + tokenlist_to_string(params) + ")";
}

ExpressionListNode::ExpressionListNode(
        Token token, std::vector<BaseNode *> children)
        : BaseNode(token, children) {}

void ExpressionListNode::serialize(Serializer &serializer) const {
    for (BaseNode const *child : get_children()) {
        child->serialize(serializer);
    }
}

IfNode::IfNode(Token token, BaseNode *cond, BaseNode *case_true)
        : BaseNode(token, {cond, case_true}) {}

void IfNode::serialize(Serializer &serializer) const {
    Label label_end = serializer.get_label();
    get_first()->serialize(serializer);
    serializer.add_instr(OpCode::BrFalse, label_end, true);
    get_second()->serialize(serializer);
    serializer.add_label(label_end);
}

ForLoopNode::ForLoopNode(Token token, BaseNode *init, BaseNode *cond, 
        BaseNode *post, BaseNode *body)
        : BaseNode(token, {init, cond, post, body}) {}

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

ReturnNode::ReturnNode(Token token, BaseNode *child)
        : BaseNode(token, {child}) {}

void ReturnNode::serialize(Serializer &serializer) const {
    get_first()->serialize(serializer);
    serializer.add_instr(OpCode::Ret);
}

DeclarationNode::DeclarationNode(Token token, Token ident, BaseNode *expr)
        : BaseNode(token, {expr}), ident(ident) {}

void DeclarationNode::resolve_symbols_second_pass(
        Serializer &serializer, SymbolMap &global_scope, 
        SymbolMap &enclosing_scope, SymbolMap &current_scope) {
    if (get_first() != nullptr) {
        get_first()->resolve_symbols_second_pass(serializer, global_scope, 
                enclosing_scope, current_scope);
    }
    set_id(serializer.declare_symbol(
            ident.get_data(), current_scope, StorageType::Relative));
    serializer.add_to_container(get_id());
}

void DeclarationNode::serialize(Serializer &serializer) const {
    if (get_first() != nullptr) {
        SymbolEntry entry = serializer.get_symbol_entry(get_id());
        serializer.add_instr(OpCode::LoadAddrRel, entry.value);
        get_first()->serialize(serializer);
        serializer.add_instr(OpCode::Binary, FuncCode::Assign);
        serializer.add_instr(OpCode::Pop);
    }
}

std::string DeclarationNode::get_label() const {
    return get_token().get_data() + " " + ident.get_data();
}

ExpressionStatementNode::ExpressionStatementNode(BaseNode *child)
        : BaseNode(Token::synthetic("<expr-stmt>"), {child}) {}

void ExpressionStatementNode::serialize(Serializer &serializer) const {
    get_first()->serialize(serializer);
    serializer.add_instr(OpCode::Pop);
}

AliasNode::AliasNode(Token token, Token alias, Token source)
        : BaseNode(token, {}), alias(alias), source(source) {}

void AliasNode::resolve_symbols_second_pass(
        Serializer &serializer, SymbolMap &global_scope, 
        SymbolMap &enclosing_scope, SymbolMap &current_scope) {
    SymbolId source_id = lookup_symbol(source.get_data(), global_scope, 
            enclosing_scope, current_scope);
    set_id(serializer.declare_symbol(alias.get_data(), current_scope, 
            StorageType::Alias, source_id));
}

void AliasNode::serialize(Serializer &) const {}

std::string AliasNode::get_label() const {
    return get_token().get_data() + " " + alias.get_data() 
            + " for " + source.get_data();
}
