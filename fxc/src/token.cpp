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
    std::string type_string = Token::type_string(type);
    if (data.empty()) {
        return type_string;
    }
    return type_string + ": " + data;
}

std::string Token::type_string(TokenType type) {
    switch (type) {
        case TokenType::Identifier:
            return "identifier";
        case TokenType::IntLit:
            return "number";
        case TokenType::Operator:
            return "operator";
        case TokenType::Separator:
            return "separator";
        case TokenType::Synthetic:
            return "synthetic";
        case TokenType::Null:
            return "null";
        case TokenType::EndOfFile:
            return "end of file";
        default:
            return "errortype";
    }
}