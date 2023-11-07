#include "tree.hpp"
#include <iostream>

Node::Node()
        : token(), children() {}

Node::Node(Token token, int arity, std::vector<Node *> children) 
        : token(token), arity(arity), children(children) {}

Node::~Node() {
    for (Node * const node : children) {
        delete node;
    }
}

void Node::translate(Intermediate &intermediate) const {
    translate_dispatch(intermediate);
    intermediate.add_instr(Instruction(OpCode::SysCall, FuncCode::Exit));
}

void Node::print(Node *node, std::string const labelPrefix, 
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
        Node::print(node->children[i], 
                branchPrefix + "\u251C\u2500\u2500\u2500", 
                branchPrefix + "\u2502   ");
    }
    Node::print(node->children[i], 
            branchPrefix + "\u2514\u2500\u2500\u2500", 
            branchPrefix + "    ");
}

void Node::translate_dispatch(Intermediate &intermediate) const {
    switch (arity) {
        case 0:
            translate_value(intermediate);
            break;
        case 1:
            translate_unary(intermediate);
            break;
        case 2:
            translate_binary(intermediate);
            break;
        case 3:
            translate_ternary(intermediate);
            break;
        case -1:
            for (Node * const child : children) {
                child->translate_dispatch(intermediate);
            }
        default:
            break;
    }
}

void Node::translate_value(Intermediate &intermediate) const {
    uint32_t value = std::stoi(token.get_data());
    intermediate.add_instr(Instruction(OpCode::Push, value & 0xFFFF));
    if (value >> 16) {
        intermediate.add_instr(Instruction(OpCode::SetHi, value >> 16));
    }
}

void Node::translate_unary(Intermediate &intermediate) const {
    children[0]->translate_dispatch(intermediate);
    if (token.get_data() == "-") {
        intermediate.add_instr(Instruction(OpCode::Unary, FuncCode::Neg));
    } else {
        throw std::runtime_error("Not implemented");
    }
}

void Node::translate_binary(Intermediate &intermediate) const {
    FuncCode funccode;
    std::string data = token.get_data();
    if (data == "+") {
        funccode = FuncCode::Add;
    } else if (data == "-") {
        funccode = FuncCode::Sub;
    } else if (data == "*") {
        funccode = FuncCode::Mul;
    } else if (data == "/") {
        funccode = FuncCode::Div;
    } else if (data == "%") {
        funccode = FuncCode::Mod;
    } else {
        throw std::runtime_error("Not implemented");
    }
    children[0]->translate_dispatch(intermediate);
    children[1]->translate_dispatch(intermediate);
    intermediate.add_instr(Instruction(OpCode::Binary, funccode));
}

void Node::translate_ternary(Intermediate &intermediate) const {
    children[2]->translate_dispatch(intermediate);
}
