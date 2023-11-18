#include "tree.hpp"
#include <iostream>

BaseNode::BaseNode(uint32_t arity, Token token, std::vector<BaseNode *> children)
        : token(token), children(children) {
    if (arity != children.size()) {
        throw std::runtime_error(
                "Syntax tree arity mismatch: expected " + std::to_string(arity) 
                + ", got " + std::to_string(children.size()));
    }
}

BaseNode::BaseNode(Token token, std::vector<BaseNode *> children)
        : token(token), children(children) {}

BaseNode::~BaseNode() {
    for (BaseNode const *child : children) {
        delete child;
    }
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
    std::cerr << labelPrefix << node->token.get_data() << std::endl;
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
    serializer.add_instr(OpCode::Push);
    serializer.add_data(value);
}

VariableNode::VariableNode(Token token, std::vector<BaseNode *> children)
        : BaseNode(0, token, children) {}

void VariableNode::serialize(Serializer &) const {
    throw std::runtime_error("not implemented");
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

BlockNode::BlockNode(Token token, std::vector<BaseNode *> children)
        : BaseNode(token, children) {}

void BlockNode::serialize(Serializer &serializer) const {
    for (BaseNode const *child : get_children()) {
        child->serialize(serializer);
    }
}

FunctionNode::FunctionNode(Token token, std::vector<BaseNode *> children)
        : BaseNode(3, token, children) {}

void FunctionNode::serialize(Serializer &serializer) const {
    serializer.attach_entry_label();
    get_third()->serialize(serializer);
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
