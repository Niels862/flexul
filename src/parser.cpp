#include "parser.hpp"
#include <iostream>

Parser::Parser() {}

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
T Parser::add(T node) {
    for (BaseNode *child : node->get_children()) {
        adopt(child);
    }
    trees.insert(node);
    return node;
}

void Parser::adopt(BaseNode *node) {
    if (node == nullptr) {
        return;
    }
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
                + token.to_string() + "'");
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

Token Parser::expect_token(Token const &other) {
    Token token = curr_token;
    if (token != other) {
        throw std::runtime_error(
                "Expected " + other.to_string()
                + ", got " + token.to_string());
    }
    get_token();
    return token;
}

Token Parser::accept_data(std::string const &data) {
    Token token = curr_token;
    if (token.get_data() != data) {
        return Token::null();
    }
    get_token();
    return token;
}

Token Parser::accept_type(TokenType type) {
    Token token = curr_token;
    if (token.get_type() != type) {
        return Token::null();
    }
    get_token();
    return token;
}

BaseNode *Parser::parse_filebody() {
    std::vector<BaseNode *> nodes;
    while (curr_token.get_type() != TokenType::EndOfFile) {
        nodes.push_back(parse_function_declaration());
    }
    return add(new BlockNode(nodes));
}

BaseNode *Parser::parse_function_declaration() {
    Token fn_token = expect_data("fn");
    Token ident = expect_type(TokenType::Identifier);
    expect_data("(");
    std::vector<Token> param_list = parse_param_declaration(
            Token(TokenType::Separator, ")"));
    BaseNode *body = parse_braced_block(false);
    return add(new FunctionNode(fn_token, ident, param_list, body));
}

BaseNode *Parser::parse_param_list(Token const &end_token) {
    std::vector<BaseNode *> params;

    if (curr_token == end_token) {
        get_token();
    } else {
        while (true) {
            params.push_back(parse_expression());
            if (curr_token.get_data() == ",") {
                get_token();
            } else {
                expect_token(end_token);
                break;
            }
        }
    }
    return add(new ExpressionListNode(Token::synthetic("<params>"), params));
}

std::vector<Token> Parser::parse_param_declaration(Token const &end_token) {
    std::vector<Token> params;

    if (curr_token == end_token) {
        get_token();
    } else {
        while (true) {
            params.push_back(expect_type(TokenType::Identifier));
            if (curr_token.get_data() == ",") {
                get_token();
            } else {
                expect_token(end_token);
                break;
            }
        }
    }
    return params;
}

BaseNode *Parser::parse_braced_block(bool is_scope) {
    std::vector<BaseNode *> statements;
    expect_data("{");
    while (curr_token.get_data() != "}") {
        statements.push_back(parse_statement());
    }
    get_token();
    if (is_scope) {
        return add(new ScopedBlockNode(statements));
    }
    return add(new BlockNode(statements));
}

BaseNode *Parser::parse_statement() {
    BaseNode *node;
    Token token = curr_token;
    if (token.get_data() == "if") {
        node = parse_if_else();
    } else if (token.get_data() == "for") {
        node = parse_for();
    } else if (token.get_data() == "while") {
        node = parse_while();
    } else if (token.get_data() == "{") {
        node = parse_braced_block(true);
    } else if (token.get_data() == ";") {
        node = add(new EmptyNode());
        get_token();
    } else {
        if (accept_data("return")) {
            node = add(new ReturnNode(token, {parse_expression()}));
        } else if (token.get_data() == "var") {
            node = parse_declaration();
        } else if (token.get_data() == "alias") {
            node = parse_alias();
        } else {
            node = add(new ExpressionStatementNode(parse_expression()));
        }
        expect_data(";");
    }
    return node;
}

BaseNode *Parser::parse_if_else() {
    Token token = expect_data("if"); // TODO: keywords
    expect_data("(");
    BaseNode *cond = parse_expression();
    expect_data(")");
    BaseNode *body_true = parse_statement();
    if (accept_data("else")) {
        BaseNode *body_false = parse_statement();
        return add(new IfElseNode(token, cond, body_true, body_false));
    }
    return add(new IfNode(token, cond, body_true));
}

BaseNode *Parser::parse_for() {
    Token token = expect_data("for");
    expect_data("(");
    BaseNode *init = add(new ExpressionStatementNode(parse_expression()));
    expect_data(";");
    BaseNode *cond = parse_expression();
    expect_data(";");
    BaseNode *post = add(new ExpressionStatementNode(parse_expression()));
    expect_data(")");
    BaseNode *body = parse_statement();
    return add(new ForLoopNode(token, init, cond, post, body));
}

BaseNode *Parser::parse_while() {
    Token token = expect_data("while");
    expect_data("(");
    BaseNode *cond = parse_expression();
    expect_data(")");
    BaseNode *body = parse_statement();
    return add(new ForLoopNode(token, add(new EmptyNode()), cond, 
            add(new EmptyNode()), body));
}

BaseNode *Parser::parse_declaration() {
    std::vector<BaseNode *> nodes;
    Token token = expect_data("var");

    do {
        Token ident = expect_type(TokenType::Identifier);
        if (accept_data("=")) {
            nodes.push_back(add(new DeclarationNode(
                    token, ident, parse_expression())));
        } else {
            nodes.push_back(add(new DeclarationNode(
                    token, ident, nullptr)));
        }
    } while (accept_data(","));

    if (nodes.size() == 1) {
        return nodes[0];
    }
    return add(new BlockNode(nodes));
}

BaseNode *Parser::parse_alias() {
    std::vector<BaseNode *> nodes;
    Token token = expect_data("alias");

    do {
        Token alias = expect_type(TokenType::Identifier);
        expect_data("for");
        Token source = expect_type(TokenType::Identifier);
        nodes.push_back(add(new AliasNode(token, alias, source)));
    } while (accept_data(","));

    if (nodes.size() == 1) {
        return nodes[0];
    }
    return add(new BlockNode(nodes));
}

BaseNode *Parser::parse_expression() {
    if (curr_token.get_data() == "lambda") {
        return parse_lambda();
    }
    return parse_assignment();
}

BaseNode *Parser::parse_assignment() {
    BaseNode *left = parse_ternary();
    Token token = curr_token;
    if (accept_data("=")) {
        if (!left->is_lvalue()) {
            throw std::runtime_error("Expected lvalue");
        }
        return add(new BinaryNode(token, left, parse_expression()));
    }
    return left;
}

BaseNode *Parser::parse_lambda() {
    Token token = expect_data("lambda");
    std::vector<Token> params = parse_param_declaration(Token(TokenType::Operator, ":"));
    BaseNode *body = parse_expression();
    return add(new LambdaNode(token, params, 
            add(new ReturnNode(Token::synthetic("<lambda-return>"), body))));
}

BaseNode *Parser::parse_ternary() {
    BaseNode *cond = parse_or();
    Token token = curr_token;
    if (accept_data("?")) {
        BaseNode *expr_true = parse_ternary();
        expect_data(":");
        BaseNode *expr_false = parse_ternary();
        return add(new IfElseNode(token, cond, expr_true, expr_false));
    }
    return cond;
}

BaseNode *Parser::parse_or() {
    BaseNode *left = parse_and();
    Token token = curr_token;
    while (accept_data("||")) {
        left = add(new BinaryNode(token, left, parse_and()));
        token = curr_token;
    }
    return left;
}

BaseNode *Parser::parse_and() {
    BaseNode *left = parse_equality_1();
    Token token = curr_token;
    while (accept_data("&&")) {
        left = add(new BinaryNode(token, left, parse_equality_1()));
        token = curr_token;
    }
    return left;
}

BaseNode *Parser::parse_equality_1() {
    BaseNode *left = parse_equality_2();
    Token token = curr_token;
    while (accept_data("==") || accept_data("!=")) {
        left = add(new BinaryNode(token, left, parse_equality_2()));
        token = curr_token;
    }
    return left;
}

BaseNode *Parser::parse_equality_2() {
    BaseNode *left = parse_sum();
    Token token = curr_token;
    while (accept_data("<") || accept_data(">") || accept_data("<=") 
            || accept_data(">=")) {
        left = add(new BinaryNode(token, left, parse_sum()));
        token = curr_token;
    }
    return left;
}

BaseNode *Parser::parse_sum() {
    BaseNode *left = parse_term();
    Token token = curr_token;
    while (accept_data("+") || accept_data("-")) {
        left = add(new BinaryNode(token, left, parse_term()));
        token = curr_token;
    }
    return left;
}

BaseNode *Parser::parse_term() {
    BaseNode *left = parse_value();
    Token token = curr_token;
    while (accept_data("*") || accept_data("/") || accept_data("%")) {
        left = add(new BinaryNode(token, left, parse_value()));
        token = curr_token;
    }
    return left;
}

BaseNode *Parser::parse_value() {
    Token token = curr_token;
    BaseNode *expression;
    if (accept_data("+") || accept_data("-") || accept_data("&") 
            || accept_data("*")) {
        BaseNode *value = parse_value();
        if (token.get_data() == "&" && !value->is_lvalue()) {
            throw std::runtime_error("Expected lvalue");
        }
        expression = add(new UnaryNode(token, value));
    } else if (accept_type(TokenType::IntLit)) {
        expression = add(new IntLitNode(token));
    } else if (accept_type(TokenType::Identifier)) {
        expression = add(new VariableNode(token));
    } else if (accept_data("(")) {
        expression = parse_expression();
        expect_data(")");
    } else {
        throw std::runtime_error("Expected value, got " + token.to_string());
    }
    while (accept_data("(")) {
        BaseNode *param_list = parse_param_list(
                Token(TokenType::Separator, ")"));
        expression = add(new CallNode(expression, param_list));
    }
    return expression;
}
