#include "token.hpp"

Token::Token() 
        : type(TokenType::Null), data("") {}

Token::Token(TokenType type)
        : type(type), data("") {}

Token::Token(TokenType type, std::string data) 
        : type(type), data(data) {}

TokenType Token::get_type() const {
    return type;
}

std::string Token::get_data() const {
    return data;
}

std::string Token::to_string() const {
    switch (type) {
        case TokenType::Identifier:
            return "ID: " + data;
        case TokenType::IntLit:
            return "NUM: " + data;
        case TokenType::Operator:
            return "OP: " + data;
        case TokenType::Separator:
            return "SEP: " + data;
        case TokenType::Null:
            return "NULL";
        case TokenType::EndOfFile:
            return "EOL";
        default:
            return "ERRTOKEN";
    }
}