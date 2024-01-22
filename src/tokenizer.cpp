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
    {"return", TokenType::Return},
    {"include", TokenType::Include},
    {"if", TokenType::If},
    {"else", TokenType::Else},
    {"while", TokenType::While},
    {"for", TokenType::For},
    {"lambda", TokenType::Lambda},
    {"var", TokenType::Var},
    {"alias", TokenType::Alias}
};

Tokenizer::Tokenizer()
        : syntax_map(), text(), i(0) {}

Tokenizer::Tokenizer(std::string const &filename)
        : syntax_map(default_syntax_map), text(), i(0) {
    std::string include_name = 
            filename + (endswith(filename, ".fx") ? "" : ".fx");
    std::ifstream file(filename);
    if (!file) {
        file = std::ifstream("std/" + include_name);
    }
    if (!file) {
        throw std::runtime_error("Could not open file: " + filename);
    }
    text.assign(
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
    c = text[i];
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
    return i >= text.length();
}

void Tokenizer::cleanup() {
    char c;
    while (!eof()) {
        c = text[i];
        if (c == '#') {
            while (!eof()) {
                c = text[i];
                if (c == '\n') {
                    break;
                }
                i++;
            }
        }
        if (c != ' ' && c != '\t' && c != '\n' && c != '\r') {
            return;
        }
        i++;
    }
}

void Tokenizer::assert_no_newline() const {
    if (text[i] == '\n' || text[i] == '\r') {
        throw std::runtime_error("Unexpected newline");
    }
}

Token Tokenizer::get_identifier() {
    size_t start = i;
    std::string identifier;
    do {
        i++;
    } while (std::isalpha(text[i]) || std::isdigit(text[i]) || text[i] == '_');
    identifier = text.substr(start, i - start);
    SyntaxMap::const_iterator iter = syntax_map.find(identifier);
    if (iter == syntax_map.end()) {
        return Token(TokenType::Identifier, identifier);
    }
    return Token(iter->second, identifier);
}

Token Tokenizer::get_intlit() {
    size_t start = i;
    do {
        i++;
    } while (std::isdigit(text[i]));
    return Token(TokenType::IntLit, text.substr(start, i - start));
}

Token Tokenizer::get_charlit() {
    size_t start = i;
    do {
        i++;
        assert_no_newline();
    } while (text[i] != '\'');
    i++;
    return Token(TokenType::IntLit, text.substr(start, i - start));
}

Token Tokenizer::get_operator() {
    size_t start = i;
    do {
        i++;
    } while (is_op_char(text[i]));
    return Token(TokenType::Operator, text.substr(start, i - start));
}

Token Tokenizer::get_separator() {
    Token token(TokenType::Separator, text.substr(i, 1));
    i++;
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

