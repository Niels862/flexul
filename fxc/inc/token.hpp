#ifndef FLEXUL_TOKEN_HPP
#define FLEXUL_TOKEN_HPP

#include <string>

enum class TokenType {
    Null, Identifier, IntLit, Keyword, Operator, Separator, 
    Synthetic, EndOfFile
};

class Token {
public:
    Token();
    Token(TokenType type);
    Token(TokenType type, std::string data);
    TokenType get_type() const;
    std::string get_data() const;
    std::string to_string() const;
    bool is_synthetic(std::string const &cmp_data) const;
    static std::string type_string(TokenType type);
private:
    TokenType type;
    std::string data;
};

#endif
