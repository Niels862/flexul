#include "token.hpp"
#include <stdexcept>

Token::Token() 
        : m_type(TokenType::Null), m_data("") {}

Token::Token(TokenType type)
        : m_type(type), m_data("") {}

Token::Token(TokenType type, std::string data) 
        : m_type(type), m_data(data) {}

Token Token::synthetic(std::string data) {
    return Token(TokenType::Synthetic, data);
}

Token Token::null() {
    return Token(TokenType::Null);
}

TokenType Token::type() const {
    return m_type;
}

std::string Token::data() const {
    return m_data;
}

std::string Token::to_string() const {
    std::string type_string = Token::type_string(m_type);
    if (m_data.empty()) {
        return type_string;
    }
    return type_string + ": '" + m_data + "'";
}

uint32_t Token::to_int() const {
    if (m_data.size() >= 3 && m_data[0] == '\'' && m_data[m_data.size() - 1] == '\'') {
        if (m_data.size() == 3) {
            return m_data[1];
        } else if (m_data.size() == 4 && m_data[1] == '\\') {
            switch (m_data[2]) {
                case 'n':
                    return '\n';
                case 'r':
                    return '\r';
                case 't':
                    return '\t';
                case '\'':
                case '\"':
                case '\\':
                    return m_data[2];
                case '0':
                    return 0;
                default:
                    break;
            }
        } else if (m_data.size() == 5 && m_data[1] == '\\' && m_data[2] == 'x') {
            if (std::isxdigit(m_data[3]) && std::isxdigit(m_data[4])) {
                uint32_t n = 0;
                size_t i;
                for (i = 0; i < 2; i++) {
                    char c = m_data[3 + i];
                    if (std::isdigit(c)) {
                        n = 16 * n + c - '0';
                    } else {
                        n = 16 * n + std::tolower(c) - 'a';
                    }
                }
                return n;
            }
        }
        throw std::runtime_error("Unrecognized char literal: " + m_data);
    }
    try {
        return std::stoi(m_data);
    } catch (std::exception const &e) {
        throw std::runtime_error(
                "Could not convert string to int: " + m_data);
    }
}

bool Token::is_synthetic(std::string const &cmp_data) const {
    return m_type == TokenType::Synthetic && m_data == cmp_data;
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
        case TokenType::Function:
            return "function";
        case TokenType::Inline:
            return "inline";
        case TokenType::Return:
            return "return";
        case TokenType::Include:
            return "include";
        case TokenType::If:
            return "if";
        case TokenType::Else:
            return "else";
        case TokenType::While:
            return "while";
        case TokenType::For:
            return "for";
        case TokenType::Lambda:
            return "lambda";
        case TokenType::Var:
            return "var";
        default:
            return "errortype";
    }
}

bool Token::operator ==(Token const &other) const {
    return m_type == other.m_type && m_data == other.m_data;
}

bool Token::operator !=(Token const &other) const {
    return m_type != other.m_type || m_data != other.m_data;
}

Token::operator bool() const {
    return m_type != TokenType::Null;
}

std::string tokenlist_to_string(std::vector<Token> const &tokens, 
        std::string const &sep) {
    std::string str = "";
    size_t i;
    for (i = 0; i < tokens.size(); i++) {
        if (i) {
            str += sep;
        }
        str += tokens[i].data();
    }
    return str;
}
