#include "parser.hpp"
#include "utils.hpp"
#include <iostream>

Parser::Parser() {}

Parser::Parser(std::string const &filename) {
    include_file(filename);
}

std::unique_ptr<BaseNode> Parser::parse() {
    std::unique_ptr<BaseNode> root = parse_filebody();
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

std::unique_ptr<BaseNode> Parser::parse_filebody() {
    std::vector<std::unique_ptr<BaseNode>> nodes;
    std::unique_ptr<BaseNode> node;
    while (!check_type(TokenType::EndOfFile)) {
        node = nullptr;
        if (check_type(TokenType::Include)) {
            parse_include();
        } else if (check_type(TokenType::Function)) {
            node = parse_function_declaration();
        } else if (check_type(TokenType::Inline)) {
            node = parse_inline_declaration();
        } else if (check_type(TokenType::TypeDef)) {
            node = parse_type_declaration();
        } else if (check_type(TokenType::Var)) {
            node = parse_var_declaration();
            expect_data(";");
        } else {
            throw std::runtime_error("Expected declaration");
        }
        if (node != nullptr) {
            nodes.push_back(std::move(node));
        }
    }
    return std::make_unique<BlockNode>(std::move(nodes));
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

std::unique_ptr<BaseNode> Parser::parse_function_declaration() {
    Token fn_token = expect_type(TokenType::Function);
    Token ident = accept_type(TokenType::Identifier);
    if (!ident) {
        ident = expect_type(TokenType::Operator);
    }
    CallableSignature signature = parse_param_declaration();
    std::unique_ptr<BaseNode> body = parse_braced_block(false);
    return std::make_unique<FunctionNode>(fn_token, ident, 
            std::move(signature), std::move(body));
}

std::unique_ptr<BaseNode> Parser::parse_inline_declaration() {
    Token inline_token = expect_type(TokenType::Inline);
    Token ident = accept_type(TokenType::Identifier);
    if (!ident) {
        ident = expect_type(TokenType::Operator);
    }
    CallableSignature signature = parse_param_declaration();
    expect_data(":");
    std::unique_ptr<BaseNode> body = parse_expression();
    expect_data(";");
    return std::make_unique<InlineNode>(inline_token, ident, 
            std::move(signature), std::move(body));
}

std::unique_ptr<ExpressionListNode> Parser::parse_param_list() {
    std::vector<std::unique_ptr<BaseNode>> params;

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
    return std::make_unique<ExpressionListNode>(
            Token::synthetic("<params>"), std::move(params));
}

CallableSignature Parser::parse_param_declaration() {
    std::vector<Token> params;
    std::vector<std::unique_ptr<TypeNode>> type_list;
    std::unique_ptr<TypeNode> return_type, param_type;

    expect_data("(");
    if (!accept_data(")")) {
        while (true) {
            params.push_back(expect_type(TokenType::Identifier));
            if (accept_data(":")) {
                param_type = parse_type();
            } else {
                param_type = nullptr;
            }
            type_list.push_back(std::move(param_type));
            if (!accept_data(",")) {
                expect_data(")");
                break;
            }
        }
    }
    if (accept_data("->")) {
        return_type = parse_type();
    } else {
        return_type = nullptr;
    }
    return CallableSignature(params, std::make_unique<CallableTypeNode>(
            Token::synthetic("->"), 
            std::make_unique<TypeListNode>(std::move(type_list)), 
            std::move(return_type)));
}

std::unique_ptr<BaseNode> Parser::parse_braced_block(bool is_scope) {
    std::vector<std::unique_ptr<BaseNode> > statements;
    expect_data("{");
    while (m_curr_token.data() != "}") {
        statements.push_back(parse_statement());
    }
    get_token();
    if (is_scope) {
        return std::make_unique<ScopedBlockNode>(std::move(statements));
    }
    return std::make_unique<BlockNode>(std::move(statements));
}

std::unique_ptr<BaseNode> Parser::parse_type_declaration() {
    Token token = expect_type(TokenType::TypeDef);
    Token ident = expect_type(TokenType::Identifier);
    expect_data(";");
    return std::make_unique<TypeDeclarationNode>(token, ident);
}

std::unique_ptr<TypeNode> Parser::parse_type() {
    Token ident;
    std::vector<std::unique_ptr<TypeNode>> type_list;
    if (ident = accept_type(TokenType::Identifier)) {
        std::unique_ptr<TypeNode> node = std::make_unique<NamedTypeNode>(ident);
        if (check_data("->")) {
            type_list.push_back(std::move(node));
        } else {
            return node;
        }
    } else if (expect_data("(")) {
        if (!accept_data(")")) {
            while (true) {
                type_list.push_back(parse_type());
                if (!accept_data(",")) {
                    expect_data(")");
                    break;
                }
            }
        }
    }
    Token token = expect_data("->");
    return std::make_unique<CallableTypeNode>(
            token, std::make_unique<TypeListNode>(std::move(type_list)), 
            parse_type());
}

std::unique_ptr<BaseNode> Parser::parse_statement() {
    std::unique_ptr<BaseNode> node;
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
        node = std::make_unique<EmptyNode>();
        get_token();
    } else {
        if (accept_type(TokenType::Return)) {
            node = std::make_unique<ReturnNode>(token, parse_expression());
        } else if (check_type(TokenType::Var)) {
            node = parse_var_declaration();
        } else {
            node = std::make_unique<ExpressionStatementNode>(
                    parse_expression());
        }
        expect_data(";");
    }
    return node;
}

std::unique_ptr<BaseNode> Parser::parse_if_else() {
    Token token = expect_type(TokenType::If);
    expect_data("(");
    std::unique_ptr<BaseNode> cond = parse_expression();
    expect_data(")");
    std::unique_ptr<BaseNode> body_true = parse_statement();
    if (accept_type(TokenType::Else)) {
        std::unique_ptr<BaseNode> body_false = parse_statement();
        return std::make_unique<IfElseNode>(token, 
                std::move(cond), std::move(body_true), std::move(body_false));
    }
    return std::make_unique<IfNode>(token, 
            std::move(cond), std::move(body_true));
}

std::unique_ptr<BaseNode> Parser::parse_for() {
    Token token = expect_type(TokenType::For);
    expect_data("(");
    std::unique_ptr<BaseNode> init = 
            std::make_unique<ExpressionStatementNode>(parse_expression());
    expect_data(";");
    std::unique_ptr<BaseNode> cond = parse_expression();
    expect_data(";");
    std::unique_ptr<BaseNode> post = 
            std::make_unique<ExpressionStatementNode>(parse_expression());
    expect_data(")");
    std::unique_ptr<BaseNode> body = parse_statement();
    return std::make_unique<ForLoopNode>(token, 
            std::move(init), std::move(cond), 
            std::move(post), std::move(body));
}

std::unique_ptr<BaseNode> Parser::parse_while() {
    Token token = expect_type(TokenType::While);
    expect_data("(");
    std::unique_ptr<BaseNode> cond = parse_expression();
    expect_data(")");
    std::unique_ptr<BaseNode> body = parse_statement();
    return std::make_unique<ForLoopNode>(token, 
            std::make_unique<EmptyNode>(),
            std::move(cond),
            std::make_unique<EmptyNode>(),
            std::move(body));
}

std::unique_ptr<BaseNode> Parser::parse_var_declaration() {
    std::vector<std::unique_ptr<BaseNode>> nodes;
    Token token = expect_type(TokenType::Var);

    do {
        Token ident = expect_type(TokenType::Identifier);
        std::unique_ptr<BaseNode> size = nullptr;
        std::unique_ptr<BaseNode> init_value = nullptr;
        if (accept_data("[")) {
            size = parse_expression();
            expect_data("]");
        }
        if (accept_data("=")) {
            init_value = parse_expression();
        }
        nodes.push_back(std::make_unique<VarDeclarationNode>(
                token, ident, std::move(size), std::move(init_value)));
    } while (accept_data(","));

    if (nodes.size() == 1) {
        return std::move(nodes[0]);
    }
    return std::make_unique<BlockNode>(std::move(nodes));
}

std::unique_ptr<BaseNode> Parser::parse_expression() {
    if (check_type(TokenType::Lambda)) {
        return parse_lambda();
    }
    return parse_assignment();
}

std::unique_ptr<BaseNode> Parser::parse_assignment() {
    static std::unordered_map<std::string, std::string> const assignments = {
        {"+=", "+"},
        {"-=", "-"},
        {"/=", "/"},
        {"*=", "*"},
        {"%=", "%"}
    };

    std::unique_ptr<BaseNode> left = parse_ternary();
    Token token = m_curr_token;
    if (accept_data("=")) {
        return std::make_unique<AssignNode>(token, 
                std::move(left), parse_expression());
    } else {
        auto iter = assignments.find(token.data());
        if (iter != assignments.end()) {
            throw std::runtime_error("not implemented");
        }
    }
    return left;
}

std::unique_ptr<BaseNode> Parser::parse_lambda() {
    Token token = expect_type(TokenType::Lambda);
    CallableSignature signature = parse_param_declaration();
    expect_data(":");
    std::unique_ptr<BaseNode> body = parse_expression();
    return std::make_unique<LambdaNode>(
            token, std::move(signature),
            std::make_unique<ReturnNode>(
                Token::synthetic("<lambda-return>"), std::move(body)));
}

std::unique_ptr<BaseNode> Parser::parse_ternary() {
    std::unique_ptr<BaseNode> cond = parse_or();
    Token token = m_curr_token;
    if (accept_data("?")) {
        std::unique_ptr<BaseNode> expr_true = parse_ternary();
        expect_data(":");
        std::unique_ptr<BaseNode> expr_false = parse_ternary();
        return std::make_unique<IfElseNode>(token, 
                std::move(cond), std::move(expr_true), std::move(expr_false));
    }
    return cond;
}

std::unique_ptr<BaseNode> Parser::parse_or() {
    std::unique_ptr<BaseNode> left = parse_and();
    Token token = m_curr_token;
    while (accept_data("||")) {
        left = std::make_unique<OrNode>(token, std::move(left), parse_and());
        token = m_curr_token;
    }
    return left;
}

std::unique_ptr<BaseNode> Parser::parse_and() {
    std::unique_ptr<BaseNode> left = parse_equality_1();
    Token token = m_curr_token;
    while (accept_data("&&")) {
        left = std::make_unique<AndNode>(token, std::move(left), parse_equality_1());
        token = m_curr_token;
    }
    return left;
}

std::unique_ptr<BaseNode> Parser::parse_equality_1() {
    std::unique_ptr<BaseNode> left = parse_equality_2();
    Token token = m_curr_token;
    while (accept_data("==") || accept_data("!=")) {
        left = CallNode::make_binary_call(token, 
                std::move(left), parse_equality_1());
        token = m_curr_token;
    }
    return left;
}

std::unique_ptr<BaseNode> Parser::parse_equality_2() {
    std::unique_ptr<BaseNode> left = parse_sum();
    Token token = m_curr_token;
    while (accept_data("<") || accept_data(">") || accept_data("<=") 
            || accept_data(">=")) {
        left = CallNode::make_binary_call(token, 
                std::move(left), parse_sum());
        token = m_curr_token;
    }
    return left;
}

std::unique_ptr<BaseNode> Parser::parse_sum() {
    std::unique_ptr<BaseNode> left = parse_term();
    Token token = m_curr_token;
    while (accept_data("+") || accept_data("-")) {
        left = CallNode::make_binary_call(token, 
                std::move(left), parse_term());
        token = m_curr_token;
    }
    return left;
}

std::unique_ptr<BaseNode> Parser::parse_term() {
    std::unique_ptr<BaseNode> left = parse_value();
    Token token = m_curr_token;
    while (accept_data("*") || accept_data("/") || accept_data("%")) {
        left = CallNode::make_binary_call(token, 
                std::move(left), parse_value());
        token = m_curr_token;
    }
    return left;
}

std::unique_ptr<BaseNode> Parser::parse_value() {
    Token token = m_curr_token;
    std::unique_ptr<BaseNode> value;
    if (accept_data("+") || accept_data("-")) {
        value = CallNode::make_unary_call(token, parse_value());
    } else if (accept_data("&")) {
        value = parse_value();
        if (token.data() == "&" && !value->is_lvalue()) {
            throw std::runtime_error("Expected lvalue");
        }
        value = std::make_unique<AddressOfNode>(token, std::move(value));
    } else if (accept_data("*")) {
        value = std::make_unique<DereferenceNode>(token, parse_value());
    } else if (accept_type(TokenType::IntLit)) {
        value = std::make_unique<IntLitNode>(token);
    } else if (accept_type(TokenType::Identifier)) {
        value = std::make_unique<VariableNode>(token);
    } else if (accept_data("(")) {
        value = parse_expression();
        expect_data(")");
    } else {
        throw std::runtime_error("Expected value, got " + token.to_string());
    }
    return parse_postfix(std::move(value));
}

std::unique_ptr<BaseNode> Parser::parse_postfix(std::unique_ptr<BaseNode> value) {
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
            value = std::make_unique<CallNode>(
                    std::move(value), parse_param_list());
        } else if (accept_data("[")) {
            std::unique_ptr<BaseNode> subscript = parse_expression();
            expect_data("]");
            value = std::make_unique<SubscriptNode>(
                    std::move(value), std::move(subscript));
        } else {
            auto iter = assignments.find(token.data());
            if (iter == assignments.end()) {
                return value;
            }
            get_token();
            throw std::runtime_error("not implemented");
        }
    }
}
