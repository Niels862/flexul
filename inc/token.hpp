#ifndef FLEXUL_TOKEN_HPP
#define FLEXUL_TOKEN_HPP

#include <string>
#include <vector>

enum class TokenType {
    Null, Identifier, IntLit, Keyword, Operator, Separator, 
    Function, Inline, TypeDef, Return, Include, If, Else, While, 
    For, Lambda, Var, EndOfFile, Synthetic
};

class Token {
public:
    Token();
    Token(TokenType type, std::size_t row, std::size_t col);
    Token(TokenType type, std::string data, std::size_t row, std::size_t col);
    static Token synthetic(std::string data);
    static Token null();
    TokenType type() const;
    std::string data() const;
    std::string to_string() const;
    uint32_t to_int() const;
    bool is_synthetic(std::string const &cmp_data) const;
    static std::string type_string(TokenType type);
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

std::string tokenlist_to_string(std::vector<Token> const &tokens, 
        std::string const &sep = ", ");

#endif
