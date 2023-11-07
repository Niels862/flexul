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

Parser::~Parser() {
    for (Node * const node : trees) {
        delete node;
    }
}

Node *Parser::parse() {
    get_token();
    Node *root = parse_filebody();
    if (get_token().get_type() != TokenType::EndOfFile) {
        throw std::runtime_error(
                "Unexpected token: " + tokenizer.get_token().to_string());
    }
    return root;
}

Node *Parser::new_leaf(Token token) {
    Node *node = new Node(token, 0, {});
    trees.insert(node);
    return node;
}

Node *Parser::new_unary(Token token, Node *first) {
    Node *node = new Node(token, 1, {first});
    adopt(first);
    trees.insert(node);
    return node;
}

Node *Parser::new_binary(Token token, Node *first, Node *second) {
    Node *node = new Node(token, 2, {first, second});
    adopt(first);
    adopt(second);
    trees.insert(node);
    return node;
}

Node *Parser::new_ternary(Token token, Node *first, Node *second, Node *third) {
    Node *node = new Node(token, 3, {first, second, third});
    adopt(first);
    adopt(second);
    adopt(third);
    trees.insert(node);
    return node;
}

Node *Parser::new_n_ary(Token token, std::vector<Node *> children) {
    Node *node = new Node(token, -1, children);
    for (Node * const child : children) {
        adopt(child);
    }
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

Token Parser::expect_data(std::string const &data) {
    Token token = curr_token;
    if (token.get_data() != data) {
        throw std::runtime_error(
                "Expected '" + data + "', got '" 
                + token.get_data() + "'");
    }
    get_token();
    return token;
}

Token Parser::expect_type(TokenType type) {
    Token token = curr_token;
    if (token.get_type() != type) {
        throw std::runtime_error(
                "Expected token of type " + Token::type_string(type) 
                + ", got " + token.to_string());
    }
    get_token();
    return token;
}

Node *Parser::parse_filebody() {
    std::vector<Node *> nodes;
    while (curr_token.get_type() != TokenType::EndOfFile) {
        nodes.push_back(parse_function_declaration());
    }
    return new_n_ary(Token(TokenType::Synthetic, "filebody"), nodes);
}

Node *Parser::parse_function_declaration() {
    Node *statement;
    Token fn_token = expect_data("fn");
    Node *identifier = new_leaf(expect_type(TokenType::Identifier));
    Node *argslist = parse_argslist(true);
    expect_data("{");
    expect_data("return");
    statement = parse_statement();
    expect_data("}");
    return new_ternary(fn_token, 
            identifier, 
            argslist, 
            new_n_ary(Token(TokenType::Synthetic, "body"), {statement}));
}

Node *Parser::parse_argslist(bool declaration) {
    std::vector<Node *> nodes;
    Node *node;
    expect_data("(");
    if (curr_token.get_data() == ")") {
        get_token();
    } else {
        while (true) {
            if (declaration) {
                node = new_leaf(expect_type(TokenType::Identifier));
            } else {
                node = parse_expression();
            }
            nodes.push_back(node);
            if (curr_token.get_data() == ",") {
                get_token();
            } else {
                expect_data(")");
                break;
            }
        }
    }
    return new_n_ary(Token(TokenType::Synthetic, "args"), nodes);
}

Node *Parser::parse_statement() {
    Node *node = parse_expression();
    expect_data(";");
    return node;
}

Node *Parser::parse_expression() {
    return parse_sum();
}

Node *Parser::parse_sum() {
    Node *left = parse_term();
    Token token = curr_token;
    while (token.get_data() == "+" || token.get_data() == "-") {
        get_token();
        left = new_binary(token, left, parse_term());
        token = curr_token;
    }
    return left;
}

Node *Parser::parse_term() {
    Node *left = parse_value();
    Token token = curr_token;
    while (token.get_data() == "*" || token.get_data() == "/" 
            || token.get_data() == "%") {
        get_token();
        left = new_binary(token, left, parse_value());
        token = curr_token;
    }
    return left;
}

Node *Parser::parse_value() {
    Token token = curr_token;
    Node *expression;
    if (token.get_type() == TokenType::IntLit 
            || token.get_type() == TokenType::Identifier) {
        get_token();
        if (curr_token.get_data() == "(") {
            return new_binary(Token(TokenType::Synthetic, "call"),
                    new_leaf(token),
                    parse_argslist(false));
        }
        return new_leaf(token);
    }
    if (token.get_data() == "-" || token.get_data() == "+") {
        get_token();
        return new_unary(token, parse_value());
    }
    if (token.get_data() == "(") {
        get_token();
        expression = parse_expression();
        expect_data(")");
        return expression;
    }
    throw std::runtime_error("Expected value, got " + token.to_string());
}
