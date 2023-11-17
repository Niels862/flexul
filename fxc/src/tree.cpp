#include "tree.hpp"
#include <iostream>

BaseNode::BaseNode(Token token, std::vector<BaseNode *> children)
        : token(token), children(children) {}

BaseNode::~BaseNode() {
    for (BaseNode const *child : children) {
        delete child;
    }
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

IntLitNode::IntLitNode(Token token)
        : BaseNode(token, {}), value(std::stoi(token.get_data())) {}

VariableNode::VariableNode(Token token)
        : BaseNode(token, {}) {}

UnaryNode::UnaryNode(Token token, BaseNode *child)
        : BaseNode(token, {child}) {}

BinaryNode::BinaryNode(Token token, BaseNode *left, BaseNode *right)
        : BaseNode(token, {left, right}) {}

BlockNode::BlockNode(std::vector<BaseNode *> children)
        : BaseNode(Token(TokenType::Synthetic, "<block>"), children) {}
