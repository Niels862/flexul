#include "parser.hpp"
#include "utils.hpp"
#include <iostream>

Parser::Parser() {}

Parser::Parser(std::string const &filename) {
    include_file(filename);
}

Parser::~Parser() {
    for (BaseNode * const node : m_trees) {
        delete node;
    }
}

BaseNode *Parser::parse() {
    BaseNode *root = parse_filebody();
    if (get_token().type() != TokenType::EndOfFile) {
        throw std::runtime_error(
                "Unexpected token: " + m_curr_token.to_string());
    }
    return root;
}

void Parser::include_file(std::string const &filename) {
    if (m_included_files.find(filename) != m_included_files.end()) {
        get_token();
    } else {
        Tokenizer tokenizer(filename);
        m_curr_token = tokenizer.get_token();
        m_tokenizers.push(tokenizer);
        m_included_files.insert(filename);
    }
}

template <typename T>
T Parser::add(T node) {
    for (BaseNode *child : node->children()) {
        adopt(child);
    }
    m_trees.insert(node);
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

BaseNode *Parser::add_link(BaseNode *link) {
    BaseNode *node = new LinkNode(link);
    m_trees.insert(node);
    return node;
}

void Parser::adopt(BaseNode *node) {
    if (node == nullptr) {
        return;
    }
    if (m_trees.find(node) == m_trees.end()) {
        throw std::runtime_error("Violation: child is not a (sub)tree");
    }
    m_trees.erase(node);
}

Token Parser::get_token() {
    if (m_tokenizers.empty()) {
        return Token(TokenType::EndOfFile);
    }
    m_curr_token = m_tokenizers.top().get_token();
    while (m_curr_token == Token(TokenType::EndOfFile)) {
        m_tokenizers.pop();
        if (m_tokenizers.empty()) {
            return m_curr_token;
        }
        m_curr_token = m_tokenizers.top().get_token();
    }
    return m_curr_token;
}

Token Parser::expect_data(std::string const &data) {
    Token token = m_curr_token;
    if (token.data() != data) {
        throw std::runtime_error(
                "Expected '" + data + "', got '" 
                + token.to_string() + "'");
    }
    get_token();
    return token;
}

Token Parser::expect_type(TokenType type) {
    Token token = m_curr_token;
    if (token.type() != type) {
        throw std::runtime_error(
                "Expected token of type " + Token::type_string(type) 
                + ", got " + token.to_string());
    }
    get_token();
    return token;
}

Token Parser::expect_token(Token const &other) {
    Token token = m_curr_token;
    if (token != other) {
        throw std::runtime_error(
                "Expected " + other.to_string()
                + ", got " + token.to_string());
    }
    get_token();
    return token;
}

Token Parser::accept_data(std::string const &data) {
    Token token = m_curr_token;
    if (token.data() != data) {
        return Token::null();
    }
    get_token();
    return token;
}

Token Parser::accept_type(TokenType type) {
    Token token = m_curr_token;
    if (token.type() != type) {
        return Token::null();
    }
    get_token();
    return token;
}

Token Parser::check_data(std::string const &data) const {
    if (m_curr_token.data() != data) {
        return Token::null();
    }
    return m_curr_token;
}

Token Parser::check_type(TokenType type) const {
    if (m_curr_token.type() != type) {
        return Token::null();
    }
    return m_curr_token;
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
        } else if (check_type(TokenType::Type)) {
            node = parse_type_declaration();
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
    std::string filename = expect_type(TokenType::Identifier).data();
    if (m_curr_token.data() != ";") {
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
    CallableSignature signature = parse_param_declaration();
    BaseNode *body = parse_braced_block(false);
    return add(new FunctionNode(fn_token, ident, signature, body));
}

BaseNode *Parser::parse_inline_declaration() {
    Token inline_token = expect_type(TokenType::Inline);
    Token ident = accept_type(TokenType::Identifier);
    if (!ident) {
        ident = expect_type(TokenType::Operator);
    }
    CallableSignature signature = parse_param_declaration();
    expect_data(":");
    BaseNode *body = parse_expression();
    expect_data(";");
    return add(new InlineNode(inline_token, ident, signature, body));
}

BaseNode *Parser::parse_param_list() {
    std::vector<BaseNode *> params;

    expect_data("(");
    if (!accept_data(")")) {
        while (true) {
            params.push_back(parse_expression());
            if (m_curr_token.data() == ",") {
                get_token();
            } else {
                expect_data(")");
                break;
            }
        }
    }
    return add(new ExpressionListNode(Token::synthetic("<params>"), params));
}

CallableSignature Parser::parse_param_declaration() {
    CallableSignature signature;

    expect_data("(");
    if (!accept_data(")")) {
        while (true) {
            signature.params.push_back(expect_type(TokenType::Identifier));
            if (accept_data(":")) {
                expect_type(TokenType::Identifier);
            }
            signature.param_types.push_back(nullptr);
            if (!accept_data(",")) {
                expect_data(")");
                break;
            }
        }
    }
    if (accept_data("->")) {
        expect_type(TokenType::Identifier);
    }
    signature.return_type = nullptr;
    return signature;
}

BaseNode *Parser::parse_braced_block(bool is_scope) {
    std::vector<BaseNode *> statements;
    expect_data("{");
    while (m_curr_token.data() != "}") {
        statements.push_back(parse_statement());
    }
    get_token();
    if (is_scope) {
        return add(new ScopedBlockNode(statements));
    }
    return add(new BlockNode(statements));
}

BaseNode *Parser::parse_type_declaration() {
    Token token = expect_type(TokenType::Type);
    Token ident = expect_type(TokenType::Identifier);
    expect_data(";");
    return add(new TypeDeclarationNode(token, ident));
}

BaseNode *Parser::parse_statement() {
    BaseNode *node;
    Token token = m_curr_token;
    if (check_type(TokenType::If)) {
        node = parse_if_else();
    } else if (check_type(TokenType::For)) {
        node = parse_for();
    } else if (check_type(TokenType::While)) {
        node = parse_while();
    } else if (token.data() == "{") {
        node = parse_braced_block(true);
    } else if (token.data() == ";") {
        node = add(new EmptyNode());
        get_token();
    } else {
        if (accept_type(TokenType::Return)) {
            node = add(new ReturnNode(token, {parse_expression()}));
        } else if (check_type(TokenType::Var)) {
            node = parse_var_declaration();
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
        nodes.push_back(add(new VarDeclarationNode(
                token, ident, size, init_value)));
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
    static std::unordered_map<std::string, std::string> const assignments = {
        {"+=", "+"},
        {"-=", "-"},
        {"/=", "/"},
        {"*=", "*"},
        {"%=", "%"}
    };

    BaseNode *left = parse_ternary();
    Token token = m_curr_token;
    if (accept_data("=")) {
        return add(new AssignNode(token, left, parse_expression()));
    } else {
        auto iter = assignments.find(token.data());
        if (iter != assignments.end()) {
            get_token();
            return add(new AssignNode(token, left, add_binary(
                    Token::synthetic(iter->second), 
                    add_link(left), parse_expression())));
        }
    }
    return left;
}

BaseNode *Parser::parse_lambda() {
    Token token = expect_type(TokenType::Lambda);
    CallableSignature signature = parse_param_declaration();
    expect_data(":");
    BaseNode *body = parse_expression();
    return add(new LambdaNode(token, signature, 
            add(new ReturnNode(Token::synthetic("<lambda-return>"), body))));
}

BaseNode *Parser::parse_ternary() {
    BaseNode *cond = parse_or();
    Token token = m_curr_token;
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
    Token token = m_curr_token;
    while (accept_data("||")) {
        left = add(new OrNode(token, left, parse_and()));
        token = m_curr_token;
    }
    return left;
}

BaseNode *Parser::parse_and() {
    BaseNode *left = parse_equality_1();
    Token token = m_curr_token;
    while (accept_data("&&")) {
        left = add(new AndNode(token, left, parse_equality_1()));
        token = m_curr_token;
    }
    return left;
}

BaseNode *Parser::parse_equality_1() {
    BaseNode *left = parse_equality_2();
    Token token = m_curr_token;
    while (accept_data("==") || accept_data("!=")) {
        left = add_binary(token, left, parse_equality_2());
        token = m_curr_token;
    }
    return left;
}

BaseNode *Parser::parse_equality_2() {
    BaseNode *left = parse_sum();
    Token token = m_curr_token;
    while (accept_data("<") || accept_data(">") || accept_data("<=") 
            || accept_data(">=")) {
        left = add_binary(token, left, parse_sum());
        token = m_curr_token;
    }
    return left;
}

BaseNode *Parser::parse_sum() {
    BaseNode *left = parse_term();
    Token token = m_curr_token;
    while (accept_data("+") || accept_data("-")) {
        left = add_binary(token, left, parse_term());
        token = m_curr_token;
    }
    return left;
}

BaseNode *Parser::parse_term() {
    BaseNode *left = parse_value();
    Token token = m_curr_token;
    while (accept_data("*") || accept_data("/") || accept_data("%")) {
        left = add_binary(token, left, parse_value());
        token = m_curr_token;
    }
    return left;
}

BaseNode *Parser::parse_value() {
    Token token = m_curr_token;
    BaseNode *value;
    if (accept_data("+") || accept_data("-")) {
        value = add_unary(token, parse_value());
    } else if (accept_data("&")) {
        value = parse_value();
        if (token.data() == "&" && !value->is_lvalue()) {
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
    static std::unordered_map<std::string, std::string> const assignments = {
        {"++", "+"},
        {"--", "-"},
        {"**", "*"},
        {"//", "/"},
        {"%%", "%"}
    };

    while (true) {
        Token token = m_curr_token;
        if (check_data("(")) {
            BaseNode *param_list = parse_param_list();
            value = add(new CallNode(value, param_list));
        } else if (accept_data("[")) {
            BaseNode *subscript = parse_expression();
            expect_data("]");
            value = add(new SubscriptNode(value, subscript));
        } else {
            auto iter = assignments.find(token.data());
            if (iter == assignments.end()) {
                return value;
            }
            get_token();
            return add(new AssignNode(token, value, add_binary(
                    Token::synthetic(iter->second),
                    add_link(value),
                    add(new IntLitNode(Token::synthetic("1"))))));
        }
    }
}
