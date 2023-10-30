#include "tokenizer.hpp"
#include <cctype>
#include <stdexcept>

Tokenizer::Tokenizer()
        : text(), i(0) {}

Tokenizer::Tokenizer(std::string const &text) 
        : text(text), i(0) {}

void Tokenizer::reset() {
    i = 0;
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
        if (c != ' ' && c != '\t' && c != '\n' && c != '\r') {
            return;
        }
        i++;
    }
}

Token Tokenizer::get_identifier() {
    size_t start = i;
    do {
        i++;
    } while (std::isalpha(text[i]) || std::isdigit(text[i]) || text[i] == '_');
    return Token(TokenType::Identifier, text.substr(start, i - start));
}

Token Tokenizer::get_intlit() {
    size_t start = i;
    do {
        i++;
    } while (std::isdigit(text[i]));
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
            || c == '>' || c == '<' || c == '.' || c == '~';
}

bool is_sep_char(char c) {
    return c == '(' || c == ')' || c == '{' || c == '}' || c == '['
            || c == '[' || c == ']' || c == ',' || c == ';';
}

