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

void Node::rename() {
    if (!token.is_synthetic("filebody")) {
        throw std::runtime_error("Rename may only be used on filebody node");
    }
    std::unordered_map<std::string, std::string> global_scope_map;
    size_t i = 0;
    for (Node * const child : children) { // First pass: rename funcs
        if (child->token.get_data() == "fn") {
            child->children[0]->assign_new_name(global_scope_map, i);
        }
    }
    for (Node * const child : children) {
        child->rename(global_scope_map, i);
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

void Node::assign_new_name(RenameMap &scope_map, size_t &i) {
    std::string new_name = "v" + std::to_string(i);
    scope_map[token.get_data()] = new_name;
    token = Token(TokenType::Identifier, new_name);
    i++;
}

void Node::rename(RenameMap &scope_map, size_t &i) {
    if (token.get_data() == "fn") {
        RenameMap func_scope_map(scope_map);
        for (Node * const child : children[1]->children) { // args
            child->assign_new_name(func_scope_map, i);
        }
        children[2]->rename(func_scope_map, i);
    } else if (token.get_type() == TokenType::Identifier) {
        RenameMap::const_iterator iter = scope_map.find(token.get_data());
        if (iter == scope_map.end()) {
            throw std::runtime_error(
                    "Undeclared variable: " + token.get_data());
        } else {
            token = Token(TokenType::Identifier, iter->second);
        }
    } else {
        for (Node * const child : children) {
            child->rename(scope_map, i);
        }
    }
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
    if (token.get_type() != TokenType::IntLit) {
        throw std::runtime_error("Not implemented");
    }
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
