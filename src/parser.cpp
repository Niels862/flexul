#include "parser.hpp"
#include "utils.hpp"
#include <iostream>

Parser::Parser() {}

Parser::Parser(std::string const &filename) {
    include_file(filename);
}

Parser::~Parser() {
    for (BaseNode * const node : trees) {
        delete node;
    }
}

BaseNode *Parser::parse() {
    BaseNode *root = parse_filebody();
    if (get_token().get_type() != TokenType::EndOfFile) {
        throw std::runtime_error(
                "Unexpected token: " + curr_token.to_string());
    }
    print_iterable(included_files);
    return root;
}

void Parser::include_file(std::string const &filename) {
    if (included_files.find(filename) != included_files.end()) {
        get_token();
    } else {
        Tokenizer tokenizer(filename);
        curr_token = tokenizer.get_token();
        tokenizers.push(tokenizer);
        included_files.insert(filename);
    }
}

template <typename T>
T Parser::add(T node) {
    for (BaseNode *child : node->get_children()) {
        adopt(child);
    }
    trees.insert(node);
    return node;
}

BaseNode *Parser::add_unary(Token const &op, BaseNode *operand) {
    return add(new CallNode(add(new VariableNode(op)), 
            add(new ExpressionListNode(
                Token::synthetic("<params>"), {operand}))));
}

BaseNode *Parser::add_binary(Token const &op, BaseNode *left, BaseNode *right) {
    return add(new CallNode(add(new VariableNode(op)), 
            add(new ExpressionListNode(
                Token::synthetic("<params>"), {left, right}))));
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
    if (tokenizers.empty()) {
        return Token(TokenType::EndOfFile);
    }
    curr_token = tokenizers.top().get_token();
    while (curr_token == Token(TokenType::EndOfFile)) {
        tokenizers.pop();
        if (tokenizers.empty()) {
            return curr_token;
        }
        curr_token = tokenizers.top().get_token();
    }
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

Token Parser::check_type(TokenType type) const {
    if (curr_token.get_type() != type) {
        return Token::null();
    }
    return curr_token;
}

BaseNode *Parser::parse_filebody() {
    std::vector<BaseNode *> nodes;
    BaseNode *node;
    while (!check_type(TokenType::EndOfFile)) {
        node = nullptr;
        if (check_type(TokenType::Include)) {
            parse_include();
        } else if (check_type(TokenType::Function)) {
            node = parse_function_declaration();
        } else if (check_type(TokenType::Inline)) {
            node = parse_inline_declaration();
        } else if (check_type(TokenType::Var)) {
            node = parse_var_declaration();
            expect_data(";");
        } else {
            throw std::runtime_error("Expected declaration");
        }
        if (node != nullptr) {
            nodes.push_back(node);
        }
    }
    return add(new BlockNode(nodes));
}

void Parser::parse_include() {
    expect_type(TokenType::Include);
    std::string filename = expect_type(TokenType::Identifier).get_data();
    if (curr_token.get_data() != ";") {
        expect_data(";");
    } else {
        include_file(filename);
    }
}

BaseNode *Parser::parse_function_declaration() {
    Token fn_token = expect_type(TokenType::Function);
    Token ident = accept_type(TokenType::Identifier);
    if (!ident) {
        ident = expect_type(TokenType::Operator);
    }
    expect_data("(");
    std::vector<Token> param_list = parse_param_declaration(
            Token(TokenType::Separator, ")"));
    BaseNode *body = parse_braced_block(false);
    return add(new FunctionNode(fn_token, ident, param_list, body));
}

BaseNode *Parser::parse_inline_declaration() {
    Token inline_token = expect_type(TokenType::Inline);
    Token ident = accept_type(TokenType::Identifier);
    if (!ident) {
        ident = expect_type(TokenType::Operator);
    }
    expect_data("(");
    std::vector<Token> param_list = parse_param_declaration(
            Token(TokenType::Separator, ")"));
    expect_data(":");
    BaseNode *body = parse_expression();
    expect_data(";");
    return add(new InlineNode(inline_token, ident, param_list, body));
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
            if (!accept_data(",")) {
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
    if (check_type(TokenType::If)) {
        node = parse_if_else();
    } else if (check_type(TokenType::For)) {
        node = parse_for();
    } else if (check_type(TokenType::While)) {
        node = parse_while();
    } else if (token.get_data() == "{") {
        node = parse_braced_block(true);
    } else if (token.get_data() == ";") {
        node = add(new EmptyNode());
        get_token();
    } else {
        if (accept_type(TokenType::Return)) {
            node = add(new ReturnNode(token, {parse_expression()}));
        } else if (check_type(TokenType::Var)) {
            node = parse_var_declaration();
        } else if (check_type(TokenType::Alias)) {
            node = parse_alias();
        } else {
            node = add(new ExpressionStatementNode(parse_expression()));
        }
        expect_data(";");
    }
    return node;
}

BaseNode *Parser::parse_if_else() {
    Token token = expect_type(TokenType::If);
    expect_data("(");
    BaseNode *cond = parse_expression();
    expect_data(")");
    BaseNode *body_true = parse_statement();
    if (accept_type(TokenType::Else)) {
        BaseNode *body_false = parse_statement();
        return add(new IfElseNode(token, cond, body_true, body_false));
    }
    return add(new IfNode(token, cond, body_true));
}

BaseNode *Parser::parse_for() {
    Token token = expect_type(TokenType::For);
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
    Token token = expect_type(TokenType::While);
    expect_data("(");
    BaseNode *cond = parse_expression();
    expect_data(")");
    BaseNode *body = parse_statement();
    return add(new ForLoopNode(token, add(new EmptyNode()), cond, 
            add(new EmptyNode()), body));
}

BaseNode *Parser::parse_var_declaration() {
    std::vector<BaseNode *> nodes;
    Token token = expect_type(TokenType::Var);

    do {
        Token ident = expect_type(TokenType::Identifier);
        BaseNode *size = nullptr;
        BaseNode *init_value = nullptr;
        if (accept_data("[")) {
            size = parse_expression();
            expect_data("]");
        }
        if (accept_data("=")) {
            init_value = parse_expression();
        }
        nodes.push_back(add(new DeclarationNode(
                token, ident, size, init_value)));
    } while (accept_data(","));

    if (nodes.size() == 1) {
        return nodes[0];
    }
    return add(new BlockNode(nodes));
}

BaseNode *Parser::parse_alias() {
    std::vector<BaseNode *> nodes;
    Token token = expect_type(TokenType::Alias);

    do {
        Token alias = expect_type(TokenType::Identifier);
        expect_type(TokenType::For);
        Token source = expect_type(TokenType::Identifier);
        nodes.push_back(add(new AliasNode(token, alias, source)));
    } while (accept_data(","));

    if (nodes.size() == 1) {
        return nodes[0];
    }
    return add(new BlockNode(nodes));
}

BaseNode *Parser::parse_expression() {
    if (check_type(TokenType::Lambda)) {
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
        return add(new AssignNode(token, left, parse_expression()));
    }
    return left;
}

BaseNode *Parser::parse_lambda() {
    Token token = expect_type(TokenType::Lambda);
    std::vector<Token> params = parse_param_declaration(
            Token(TokenType::Operator, ":"));
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
        left = add(new OrNode(token, left, parse_and()));
        token = curr_token;
    }
    return left;
}

BaseNode *Parser::parse_and() {
    BaseNode *left = parse_equality_1();
    Token token = curr_token;
    while (accept_data("&&")) {
        left = add(new AndNode(token, left, parse_equality_1()));
        token = curr_token;
    }
    return left;
}

BaseNode *Parser::parse_equality_1() {
    BaseNode *left = parse_equality_2();
    Token token = curr_token;
    while (accept_data("==") || accept_data("!=")) {
        left = add_binary(token, left, parse_equality_2());
        token = curr_token;
    }
    return left;
}

BaseNode *Parser::parse_equality_2() {
    BaseNode *left = parse_sum();
    Token token = curr_token;
    while (accept_data("<") || accept_data(">") || accept_data("<=") 
            || accept_data(">=")) {
        left = add_binary(token, left, parse_sum());
        token = curr_token;
    }
    return left;
}

BaseNode *Parser::parse_sum() {
    BaseNode *left = parse_term();
    Token token = curr_token;
    while (accept_data("+") || accept_data("-")) {
        left = add_binary(token, left, parse_term());
        token = curr_token;
    }
    return left;
}

BaseNode *Parser::parse_term() {
    BaseNode *left = parse_value();
    Token token = curr_token;
    while (accept_data("*") || accept_data("/") || accept_data("%")) {
        left = add_binary(token, left, parse_value());
        token = curr_token;
    }
    return left;
}

BaseNode *Parser::parse_value() {
    Token token = curr_token;
    BaseNode *value;
    if (accept_data("+") || accept_data("-")) {
        value = add_unary(token, parse_value());
    } else if (accept_data("&")) {
        value = parse_value();
        if (token.get_data() == "&" && !value->is_lvalue()) {
            throw std::runtime_error("Expected lvalue");
        }
        value = add(new AddressOfNode(token, value));
    } else if (accept_data("*")) {
        value = add(new DereferenceNode(token, parse_value()));
    } else if (accept_type(TokenType::IntLit)) {
        value = add(new IntLitNode(token));
    } else if (accept_type(TokenType::Identifier)) {
        value = add(new VariableNode(token));
    } else if (accept_data("(")) {
        value = parse_expression();
        expect_data(")");
    } else {
        throw std::runtime_error("Expected value, got " + token.to_string());
    }
    return parse_postfix(value);
}

BaseNode *Parser::parse_postfix(BaseNode *value) {
    if (accept_data("(")) {
        BaseNode *param_list = parse_param_list(
        Token(TokenType::Separator, ")"));
        return add(new CallNode(parse_postfix(value), param_list));
    }
    if (accept_data("[")) {
        BaseNode *subscript = parse_expression();
        expect_data("]");
        return add(new SubscriptNode(parse_postfix(value), subscript));
    }
    return value;
}
