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

Token Token::null() {
    return Token(TokenType::Null);
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

uint32_t Token::to_int() const {
    if (data.size() >= 3 && data[0] == '\'' && data[data.size() - 1] == '\'') {
        if (data.size() == 3) {
            return data[1];
        } else if (data.size() == 4 && data[1] == '\\') {
            switch (data[2]) {
                case 'n':
                    return '\n';
                case 'r':
                    return '\r';
                case 't':
                    return '\t';
                case '\'':
                case '\"':
                case '\\':
                    return data[2];
                case '0':
                    return 0;
                default:
                    break;
            }
        } else if (data.size() == 5 && data[1] == '\\' && data[2] == 'x') {
            if (std::isxdigit(data[3]) && std::isxdigit(data[4])) {
                uint32_t n = 0;
                size_t i;
                for (i = 0; i < 2; i++) {
                    char c = data[3 + i];
                    if (std::isdigit(c)) {
                        n = 16 * n + c - '0';
                    } else {
                        n = 16 * n + std::tolower(c) - 'a';
                    }
                }
                return n;
            }
        }
        throw std::runtime_error("Unrecognized char literal: " + data);
    }
    try {
        return std::stoi(data);
    } catch (std::exception const &e) {
        throw std::runtime_error(
                "Could not convert string to int: " + data);
    }
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

bool Token::operator ==(Token const &other) const {
    return type == other.type && data == other.data;
}

bool Token::operator !=(Token const &other) const {
    return type != other.type || data != other.data;
}

Token::operator bool() const {
    return type != TokenType::Null;
}

std::string tokenlist_to_string(std::vector<Token> const &tokens, 
        std::string const &sep) {
    std::string str = "";
    size_t i;
    for (i = 0; i < tokens.size(); i++) {
        if (i) {
            str += sep;
        }
        str += tokens[i].get_data();
    }
    return str;
}
