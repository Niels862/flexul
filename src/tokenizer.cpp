#include "tokenizer.hpp"
#include "utils.hpp"
#include <cctype>
#include <unordered_set>
#include <stdexcept>
#include <iostream>
#include <fstream>

SyntaxMap const &default_syntax_map = {
    {"fn", TokenType::Function},
    {"inline", TokenType::Inline},
    {"type", TokenType::Type},
    {"return", TokenType::Return},
    {"include", TokenType::Include},
    {"if", TokenType::If},
    {"else", TokenType::Else},
    {"while", TokenType::While},
    {"for", TokenType::For},
    {"lambda", TokenType::Lambda},
    {"var", TokenType::Var}
};

Tokenizer::Tokenizer()
        : m_syntax_map(), m_text(), m_i(0) {}

Tokenizer::Tokenizer(std::string const &filename)
        : m_syntax_map(default_syntax_map), m_text(), m_i(0) {
    std::string include_name = 
            filename + (endswith(filename, ".fx") ? "" : ".fx");
    std::ifstream file(filename);
    if (!file) {
        file = std::ifstream("std/" + include_name);
    }
    if (!file) {
        throw std::runtime_error("Could not open file: " + filename);
    }
    m_text.assign(
        (std::istreambuf_iterator<char>(file)),
        (std::istreambuf_iterator<char>())
    );
}

Token Tokenizer::get_token() {
    char c;
    cleanup();
    if (eof()) {
        return Token(TokenType::EndOfFile);
    }
    c = m_text[m_i];
    if (std::isalpha(c) || c == '_') {
        return get_identifier();
    }
    if (std::isdigit(c)) {
        return get_intlit();
    }
    if (c == '\'') {
        return get_charlit();
    }
    if (is_op_char(c)) {
        return get_operator();
    }
    if (is_sep_char(c)) {
        return get_separator();
    }
    throw std::runtime_error("Unrecognized character: " + std::string(1, c));
}

bool Tokenizer::eof() {
    return m_i >= m_text.length();
}

void Tokenizer::cleanup() {
    char c;
    while (!eof()) {
        c = m_text[m_i];
        if (c == '#') {
            while (!eof()) {
                c = m_text[m_i];
                if (c == '\n') {
                    break;
                }
                m_i++;
            }
        }
        if (c != ' ' && c != '\t' && c != '\n' && c != '\r') {
            return;
        }
        m_i++;
    }
}

void Tokenizer::assert_no_newline() const {
    if (m_text[m_i] == '\n' || m_text[m_i] == '\r') {
        throw std::runtime_error("Unexpected newline");
    }
}

Token Tokenizer::get_identifier() {
    size_t start = m_i;
    std::string identifier;
    do {
        m_i++;
    } while (std::isalpha(m_text[m_i]) || std::isdigit(m_text[m_i]) || m_text[m_i] == '_');
    identifier = m_text.substr(start, m_i - start);
    SyntaxMap::const_iterator iter = m_syntax_map.find(identifier);
    if (iter == m_syntax_map.end()) {
        return Token(TokenType::Identifier, identifier);
    }
    return Token(iter->second, identifier);
}

Token Tokenizer::get_intlit() {
    size_t start = m_i;
    do {
        m_i++;
    } while (std::isdigit(m_text[m_i]));
    return Token(TokenType::IntLit, m_text.substr(start, m_i - start));
}

Token Tokenizer::get_charlit() {
    size_t start = m_i;
    do {
        m_i++;
        assert_no_newline();
    } while (m_text[m_i] != '\'');
    m_i++;
    return Token(TokenType::IntLit, m_text.substr(start, m_i - start));
}

Token Tokenizer::get_operator() {
    size_t start = m_i;
    do {
        m_i++;
    } while (is_op_char(m_text[m_i]));
    return Token(TokenType::Operator, m_text.substr(start, m_i - start));
}

Token Tokenizer::get_separator() {
    Token token(TokenType::Separator, m_text.substr(m_i, 1));
    m_i++;
    return token;
}

bool is_op_char(char c) {
    return c == '+' || c == '-' || c == '*' || c == '/' || c == '%'
            || c == '&' || c == '|' || c == '^' || c == '=' || c == '!'
            || c == '>' || c == '<' || c == '.' || c == '~' || c == '?'
            || c == ':';
}

bool is_sep_char(char c) {
    return c == '(' || c == ')' || c == '{' || c == '}' || c == '['
            || c == '[' || c == ']' || c == ',' || c == ';';
}

