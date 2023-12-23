#ifndef FLEXUL_TOKEN_HPP
#define FLEXUL_TOKEN_HPP

#include <string>
#include <vector>

enum class TokenType {
    Null, Identifier, IntLit, Keyword, Operator, Separator, 
    Function, Return, If, Else, While, For, Lambda, Var, Alias,
    Synthetic, EndOfFile
};

class Token {
public:
    Token();
    Token(TokenType type);
    Token(TokenType type, std::string data);
    static Token synthetic(std::string data);
    static Token null();
    TokenType get_type() const;
    std::string get_data() const;
    std::string to_string() const;
    uint32_t to_int() const;
    bool is_synthetic(std::string const &cmp_data) const;
    static std::string type_string(TokenType type);
    bool operator ==(Token const &other) const;
    bool operator !=(Token const &other) const;
    operator bool() const;
private:
    TokenType type;
    std::string data;
};

std::string tokenlist_to_string(std::vector<Token> const &tokens, 
        std::string const &sep = ", ");

#endif
