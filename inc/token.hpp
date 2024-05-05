#ifndef FLEXUL_TOKEN_HPP
#define FLEXUL_TOKEN_HPP

#include <string>
#include <vector>
#include <iostream>
#include <unordered_map>

enum class TokenType {
    Null, Identifier, IntLit, Keyword, Operator, Separator, 
    Function, Inline, Writeback, TypeDef, Like, Return, Include, 
    If, Else, While, For, Lambda, Var, True, False, EndOfFile, Synthetic
};

std::ostream &operator <<(std::ostream &stream, TokenType const type);

std::string to_string(TokenType type);

using SyntaxMap = std::unordered_map<std::string, TokenType>;

extern SyntaxMap const default_syntax_map;

class Token {
public:
    Token();
    Token(TokenType type, std::size_t row, std::size_t col);
    Token(TokenType type, std::string data, std::size_t row, std::size_t col);
    static Token synthetic(std::string data);
    static Token null();
    TokenType type() const;
    std::string data() const;
    uint32_t to_int() const;
    bool is_synthetic(std::string const &cmp_data) const;

    friend std::string to_string(Token const &token);

    bool operator ==(Token const &other) const;
    bool operator !=(Token const &other) const;
    operator bool() const;
private:
    TokenType m_type;
    std::string m_data;
    // todo: make point at start instead of end of token
    std::size_t row;
    std::size_t col;
};

class TokenList {
public:
    TokenList();
    
    Token const &get();
    Token const &peek() const;
    Token const &look_ahead(std::size_t offset) const;
private:
    std::vector<Token> m_tokens;
};

std::string tokenlist_to_string(std::vector<Token> const &tokens, 
        std::string const &sep = ", ");

#endif
