#include "token.hpp"
#include <stdexcept>

Token::Token() 
        : type(TokenType::Null), data("") {}

Token::Token(TokenType type)
        : type(type), data("") {}

Token::Token(TokenType type, std::string data) 
        : type(type), data(data) {}

Token Token::synthetic(std::string data) {
    return Token(TokenType::Synthetic, data);
}

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
    return type_string + ": '" + data + "'";
}

bool Token::is_synthetic(std::string const &cmp_data) const {
    return type == TokenType::Synthetic && data == cmp_data;
}

std::string Token::type_string(TokenType type) {
    switch (type) {
        case TokenType::Identifier:
            return "identifier";
        case TokenType::IntLit:
            return "number";
        case TokenType::Keyword:
            return "keyword";
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

bool Token::operator ==(Token const &other) {
    return type == other.type && data == other.data;
}

bool Token::operator !=(Token const &other) {
    return type != other.type || data != other.data;
}
