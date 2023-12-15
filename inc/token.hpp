#ifndef FLEXUL_TOKEN_HPP
#define FLEXUL_TOKEN_HPP

#include <string>
#include <vector>

enum class TokenType {
    Null, Identifier, IntLit, Keyword, Operator, Separator, 
    Synthetic, EndOfFile
};

class Token {
public:
    Token();
    Token(TokenType type);
    Token(TokenType type, std::string data);
    static Token synthetic(std::string data);
    TokenType get_type() const;
    std::string get_data() const;
    std::string to_string() const;
    bool is_synthetic(std::string const &cmp_data) const;
    static std::string type_string(TokenType type);
    bool operator ==(Token const &other);
    bool operator !=(Token const &other);
private:
    TokenType type;
    std::string data;
};

std::string tokenlist_to_string(std::vector<Token> const &tokens, 
        std::string const &sep = ", ");

#endif
