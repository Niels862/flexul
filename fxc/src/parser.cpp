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
    BaseNode *param_list = parse_param_list(true);
    BaseNode *body = parse_braced_block();
    return add<FunctionNode>(fn_token, {
        identifier, param_list, body
    });
}

BaseNode *Parser::parse_param_list(bool is_declaration) {
    std::vector<BaseNode *> params;
    BaseNode *param;
    expect_data("(");
    if (curr_token.get_data() == ")") {
        get_token();
    } else {
        while (true) {
            if (is_declaration) {
                param = add<VariableNode>(expect_type(TokenType::Identifier));
            } else {
                param = parse_expression();
            }
            params.push_back(param);
            if (curr_token.get_data() == ",") {
                get_token();
            } else {
                expect_data(")");
                break;
            }
        }
    }
    return add<ExpressionListNode>(Token::synthetic("<params>"), params);
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
    if (token.get_data() == "if") {
        node = parse_if_else();
    } else if (token.get_data() == "for") {
        node = parse_for();
    } else if (token.get_data() == "while") {
        node = parse_while();
    } else if (token.get_data() == "{") {
        node = parse_braced_block();
    } else if (token.get_data() == ";") {
        node = add<EmptyNode>(Token::synthetic("<nostmt>"), {});
        get_token();
    } else {
        if (token.get_data() == "return") {
            get_token();
            node = add<ReturnNode>(token, {parse_expression()});
        } else {
            node = add<ExpressionStatementNode>(
                    Token::synthetic("<expr-stmt>"), {parse_expression()});
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
    if (curr_token.get_data() == "else") {
        get_token();
        BaseNode *body_false = parse_statement();
        return add<IfElseNode>(token, {cond, body_true, body_false});
    }
    return add<IfNode>(token, {cond, body_true});
}

BaseNode *Parser::parse_for() {
    Token token = expect_data("for");
    expect_data("(");
    BaseNode *init = parse_expression();
    expect_data(";");
    BaseNode *cond = parse_expression();
    expect_data(";");
    BaseNode *post = parse_expression();
    expect_data(")");
    BaseNode *body = parse_statement();
    return add<ForLoopNode>(token, {
        add<ExpressionStatementNode>(Token::synthetic("<init>"), {init}),
        cond,
        add<ExpressionStatementNode>(Token::synthetic("<post>"), {post}),
        body
    });
}

BaseNode *Parser::parse_while() {
    Token token = expect_data("while");
    expect_data("(");
    BaseNode *cond = parse_expression();
    expect_data(")");
    BaseNode *body = parse_statement();
    return add<ForLoopNode>(token, {
        add<EmptyNode>(Token::synthetic("<nostmt>"), {}),
        cond,
        add<EmptyNode>(Token::synthetic("<nostmt>"), {}),
        body
    });
}

BaseNode *Parser::parse_expression() {
    if (curr_token.get_data() == "lambda") {
        return parse_lambda();
    }
    return parse_ternary();
}

BaseNode *Parser::parse_lambda() {

    BaseNode *params = add<ExpressionListNode>(
            Token::synthetic("<params>"), {});
    Token token = expect_data("lambda");
    expect_data(":");
    BaseNode *body = parse_expression();
    return add<LambdaNode>(token, {
        params, 
        add<ReturnNode>(Token::synthetic("<lambda-return>"), {body})
    });
}

BaseNode *Parser::parse_ternary() {
    BaseNode *cond = parse_assignment();
    Token token = curr_token;
    if (token.get_data() == "?") {
        get_token();
        BaseNode *expr_true = parse_ternary();
        expect_data(":");
        BaseNode *expr_false = parse_ternary();
        return add<IfElseNode>(token, {cond, expr_true, expr_false});
    }
    return cond;
}

BaseNode *Parser::parse_assignment() {
    BaseNode *left = parse_or();
    Token token = curr_token;
    if (token.get_data() == "=") {
        if (!left->is_lvalue()) {
            throw std::runtime_error("Expected lvalue");
        }
        get_token();
        return add<BinaryNode>(token, {left, parse_assignment()});
    }
    return left;
}

BaseNode *Parser::parse_or() {
    BaseNode *left = parse_and();
    Token token = curr_token;
    while (token.get_data() == "||") {
        get_token();
        left = add<BinaryNode>(token, {left, parse_and()});
        token = curr_token;
    }
    return left;
}

BaseNode *Parser::parse_and() {
    BaseNode *left = parse_equality_1();
    Token token = curr_token;
    while (token.get_data() == "&&") {
        get_token();
        left = add<BinaryNode>(token, {left, parse_equality_1()});
        token = curr_token;
    }
    return left;
}

BaseNode *Parser::parse_equality_1() {
    BaseNode *left = parse_equality_2();
    Token token = curr_token;
    while (token.get_data() == "==" || token.get_data() == "!=") {
        get_token();
        left = add<BinaryNode>(token, {left, parse_equality_2()});
        token = curr_token;
    }
    return left;
}

BaseNode *Parser::parse_equality_2() {
    BaseNode *left = parse_sum();
    Token token = curr_token;
    while (token.get_data() == "<" || token.get_data() == ">"
            || token.get_data() == "<=" || token.get_data() == ">=") {
        get_token();
        left = add<BinaryNode>(token, {left, parse_sum()});
        token = curr_token;
    }
    return left;
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
    if (token.get_data() == "-" || token.get_data() == "+") {
        get_token();
        expression = add<UnaryNode>(token, {parse_value()});
    } else if (token.get_type() == TokenType::IntLit) {
        get_token();
        expression = add<IntLitNode>(token);
    } else if (token.get_type() == TokenType::Identifier) {
        get_token();
        expression = add<VariableNode>(token);
    } else if (token.get_data() == "(") {
        get_token();
        expression = parse_expression();
        expect_data(")");
    } else {
        throw std::runtime_error("Expected value, got " + token.to_string());
    }
    while (curr_token.get_data() == "(") {
        BaseNode *param_list = parse_param_list(false);
        expression = add<CallNode>(Token::synthetic("<call>"), {
            expression, param_list
        });
    }
    return expression;
}
