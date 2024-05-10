#include "token.hpp"
#include <stdexcept>

std::ostream &operator <<(std::ostream &stream, TokenType const type) {
    stream << to_string(type);
    return stream;
}

std::string to_string(TokenType type) {
    switch (type) {
        case TokenType::Identifier:
            return "identifier";
        case TokenType::IntLit:
            return "integer literal";
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
        case TokenType::Writeback:
            return "writeback";
        case TokenType::TypeDef:
            return "typedef";
        case TokenType::Like:
            return "like";
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
        case TokenType::True:
            return "true";
        case TokenType::False:
            return "false";
    }
    return "errortype";
}

SyntaxMap const default_syntax_map = {
    {"fn", TokenType::Function},
    {"inline", TokenType::Inline},
    {"writeback", TokenType::Writeback},
    {"typedef", TokenType::TypeDef},
    {"like", TokenType::Like},
    {"return", TokenType::Return},
    {"include", TokenType::Include},
    {"if", TokenType::If},
    {"else", TokenType::Else},
    {"while", TokenType::While},
    {"for", TokenType::For},
    {"lambda", TokenType::Lambda},
    {"var", TokenType::Var},
    {"true", TokenType::True},
    {"false", TokenType::False}
};

Token::Token() 
        : m_type(TokenType::Null), m_data(""), row(0), col(0) {}

Token::Token(TokenType type, std::size_t row, std::size_t col)
        : m_type(type), m_data(""), row(row), col(col) {}

Token::Token(TokenType type, std::string data, std::size_t row, std::size_t col) 
        : m_type(type), m_data(data), row(row), col(col) {}

Token Token::synthetic(std::string data) {
    return Token(TokenType::Synthetic, data, 0, 0);
}

Token Token::null() {
    return Token();
}

TokenType Token::type() const {
    return m_type;
}

std::string Token::data() const {
    return m_data;
}

uint32_t Token::to_int() const {
    if (m_data.size() >= 3 && m_data[0] == '\'' 
            && m_data[m_data.size() - 1] == '\'') {
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

std::string to_string(Token const &token) {
    return to_string(token.m_type) + ": '" + token.m_data + "' (" 
        + std::to_string(token.row) + ":" + std::to_string(token.col) + ")";
}

std::ostream &operator <<(std::ostream &stream, Token const &token) {
    stream << to_string(token);
    return stream;
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

TokenList::TokenList()
        : m_tokens() {}

Token const &TokenList::get() {
    Token const &token = m_tokens[m_p];
    if (m_p < m_tokens.size()) {
        m_p++;
    }
    return token;
}

Token const &TokenList::peek() const {
    return m_tokens[m_p];
}

Token const &TokenList::look_ahead(std::size_t offset) const {
    return m_tokens[std::min(m_p + offset, m_tokens.size() - 1)];
}

std::vector<Token>::const_iterator TokenList::begin() const {
    return m_tokens.begin();
}

std::vector<Token>::const_iterator TokenList::end() const {
    return m_tokens.end();
}

void TokenList::add(Token token) {
    m_tokens.push_back(token);
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
