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

BaseNode *Parser::new_intlit(Token token) {
    BaseNode *node = new IntLitNode(token);
    trees.insert(node);
    return node;
}

BaseNode *Parser::new_variable(Token token) {
    BaseNode *node = new IntLitNode(token);
    trees.insert(node);
    return node;
}

BaseNode *Parser::new_unary(Token token, BaseNode *first) {
    BaseNode *node = new UnaryNode(token, first);
    adopt(first);
    trees.insert(node);
    return node;
}

BaseNode *Parser::new_binary(Token token, BaseNode *first, BaseNode *second) {
    BaseNode *node = new BinaryNode(token, first, second);
    adopt(first);
    adopt(second);
    trees.insert(node);
    return node;
}

BaseNode *Parser::new_block(std::vector<BaseNode *> children) {
    BaseNode *node = new BlockNode(children);
    for (BaseNode * const child : children) {
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
        nodes.push_back(parse_statement());
    }
    return new_block(nodes);
}

// BaseNode *Parser::parse_function_declaration() {
//     BaseNode *statement;
//     Token fn_token = expect_data("fn");
//     BaseNode *identifier = new_leaf(expect_type(TokenType::Identifier));
//     BaseNode *argslist = parse_argslist(true);
//     expect_data("{");
//     expect_data("return");
//     statement = parse_statement();
//     expect_data("}");
//     return new_ternary(fn_token, 
//             identifier, 
//             argslist, 
//             new_n_ary(Token(TokenType::Synthetic, "body"), {statement}));
// }

// BaseNode *Parser::parse_argslist(bool declaration) {
//     std::vector<BaseNode *> nodes;
//     BaseNode *node;
//     expect_data("(");
//     if (curr_token.get_data() == ")") {
//         get_token();
//     } else {
//         while (true) {
//             if (declaration) {
//                 node = new_leaf(expect_type(TokenType::Identifier));
//             } else {
//                 node = parse_expression();
//             }
//             nodes.push_back(node);
//             if (curr_token.get_data() == ",") {
//                 get_token();
//             } else {
//                 expect_data(")");
//                 break;
//             }
//         }
//     }
//     return new_n_ary(Token(TokenType::Synthetic, "args"), nodes);
// }

BaseNode *Parser::parse_statement() {
    BaseNode *node = parse_expression();
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
        left = new_binary(token, left, parse_term());
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
        left = new_binary(token, left, parse_value());
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
        // if (curr_token.get_data() == "(") {
        //     return new_binary(Token(TokenType::Synthetic, "call"),
        //             new_leaf(token),
        //             parse_argslist(false));
        // }
        return new_intlit(token);
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
