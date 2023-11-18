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
    for (BaseNode * const node : trees) {
        delete node;
    }
}

BaseNode *Parser::parse() {
    get_token();
    BaseNode *root = parse_filebody();
    if (get_token().get_type() != TokenType::EndOfFile) {
        throw std::runtime_error(
                "Unexpected token: " + tokenizer.get_token().to_string());
    }
    return root;
}

template <typename T>
T *Parser::add(Token token, std::vector<BaseNode *> const &children) {
    T *node = new T(token, children);
    for (BaseNode *child : children) {
        adopt(child);
    }
    trees.insert(node);
    return node;
}

void Parser::adopt(BaseNode *node) {
    if (trees.find(node) == trees.end()) {
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

BaseNode *Parser::parse_filebody() {
    std::vector<BaseNode *> nodes;
    while (curr_token.get_type() != TokenType::EndOfFile) {
        nodes.push_back(parse_function_declaration());
    }
    return add<BlockNode>(Token(TokenType::Synthetic, "<filebody>"), nodes);
}

BaseNode *Parser::parse_function_declaration() {
    Token fn_token = expect_data("fn");
    BaseNode *identifier = add<VariableNode>(
            expect_type(TokenType::Identifier));
    BaseNode *params_list = add<BlockNode>(Token::synthetic("<params>"));
    expect_data("(");
    expect_data(")");
    BaseNode *body = parse_braced_block();
    return add<FunctionNode>(fn_token, {
        identifier, params_list, body
    });
}

BaseNode *Parser::parse_braced_block() {
    std::vector<BaseNode *> statements;
    expect_data("{");
    while (curr_token.get_data() != "}") {
        statements.push_back(parse_statement());
        if (curr_token.get_type() == TokenType::EndOfFile) {
            throw std::runtime_error("EOF before closing brace");
        }
    }
    get_token();
    return add<BlockNode>(Token(TokenType::Synthetic, "<block>"), statements);
}

BaseNode *Parser::parse_statement() {
    BaseNode *node;
    Token token = curr_token;
    if (token.get_data() == "return") {
        get_token();
        node = add<ReturnNode>(token, {parse_expression()});
    } else {
        node = parse_expression();
    }
    expect_data(";");
    return node;
}

BaseNode *Parser::parse_expression() {
    return parse_sum();
}

BaseNode *Parser::parse_sum() {
    BaseNode *left = parse_term();
    Token token = curr_token;
    while (token.get_data() == "+" || token.get_data() == "-") {
        get_token();
        left = add<BinaryNode>(token, {left, parse_term()});
        token = curr_token;
    }
    return left;
}

BaseNode *Parser::parse_term() {
    BaseNode *left = parse_value();
    Token token = curr_token;
    while (token.get_data() == "*" || token.get_data() == "/" 
            || token.get_data() == "%") {
        get_token();
        left = add<BinaryNode>(token, {left, parse_value()});
        token = curr_token;
    }
    return left;
}

BaseNode *Parser::parse_value() {
    Token token = curr_token;
    BaseNode *expression;
    if (token.get_type() == TokenType::IntLit 
            || token.get_type() == TokenType::Identifier) {
        get_token();
        return add<IntLitNode>(token);
    }
    if (token.get_data() == "-" || token.get_data() == "+") {
        get_token();
        return add<UnaryNode>(token, {parse_value()});
    }
    if (token.get_data() == "(") {
        get_token();
        expression = parse_expression();
        expect_data(")");
        return expression;
    }
    throw std::runtime_error("Expected value, got " + token.to_string());
}
