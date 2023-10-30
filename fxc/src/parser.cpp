#include "parser.hpp"
#include <iostream>

Parser::Parser() {
}

Parser::Parser(std::ifstream &file) {
    std::string text;
    text.assign(
        (std::istreambuf_iterator<char>(file)),
        (std::istreambuf_iterator<char>())
    );
    tokenizer = Tokenizer(text);
}

void Parser::reset() {
    for (Node * const node : trees) {
        delete node;
    }
    trees.clear();
    tokenizer.reset();
}

Node *Parser::parse() {
    get_token();
    Node *root;
    try {
        root = parse_filebody();
    } catch (std::exception const &e) {
        reset();
        throw;
    }
    if (get_token().get_type() != TokenType::EndOfFile) {
        reset();
        throw std::runtime_error(
                "Unexpected token: " + tokenizer.get_token().to_string());
    }
    return root;
}

Node *Parser::new_leaf(Token token) {
    Node *node = new Node(token, {});
    trees.insert(node);
    return node;
}

Node *Parser::new_unary(Token token, Node *first) {
    Node *node = new Node(token, {first});
    adopt(first);
    trees.insert(node);
    return node;
}

Node *Parser::new_binary(Token token, Node *first, Node *second) {
    Node *node = new Node(token, {first, second});
    adopt(first);
    adopt(second);
    trees.insert(node);
    return node;
}

void Parser::adopt(Node *node) {
    if (trees.find(node) == trees.end()) {
        for (Node * const subtree : trees) {
            std::cerr << "Have: " << subtree << std::endl;
            Node::print(subtree);
        }
        std::cerr << "Child: " << node << std::endl;
        Node::print(node);
        throw std::runtime_error("Violation: child is not a (sub)tree");
    }
    trees.erase(node);
}

Token Parser::get_token() {
    curr_token = tokenizer.get_token();
    return curr_token;
}

Node *Parser::parse_filebody() {
    return parse_statement();
}

Node *Parser::parse_statement() {
    Node *node = parse_sum();
    if (curr_token.get_data() != ";") {
        throw std::runtime_error("Expected ;");
    }
    return node;
}

Node *Parser::parse_sum() {
    Node *left = parse_value();
    Token token = curr_token;
    while (token.get_type() == TokenType::Operator && token.get_data() == "+") {
        get_token();
        left = new_binary(token, left, parse_value());
        token = curr_token;
    }
    return left;
}

Node *Parser::parse_value() {
    Token token = curr_token;
    if (token.get_type() == TokenType::IntLit) {
        get_token();
        return new_leaf(token);
    }
    throw std::runtime_error("Expected value, got " + token.to_string());
}
