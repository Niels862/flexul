#include "tokenizer.hpp"
#include <cctype>
#include <unordered_set>
#include <stdexcept>

std::unordered_set<std::string> keywords = {
    "fn", "return", "if", "while", "for", "lambda", "var", "alias", "rename"
};

Tokenizer::Tokenizer()
        : text(), i(0) {}

Tokenizer::Tokenizer(std::string const &text) 
        : text(text), i(0) {}

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
    if (keywords.find(identifier) == keywords.end()) {
        return Token(TokenType::Identifier, identifier);
    }
    return Token(TokenType::Keyword, identifier);
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

