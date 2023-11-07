#include <string>

#ifndef FLEXUL_TOKEN_HPP
#define FLEXUL_TOKEN_HPP

enum class TokenType {
    Null, Identifier, IntLit, Operator, Separator, Synthetic, EndOfFile
};

class Token {
public:
    Token();
    Token(TokenType type);
    Token(TokenType type, std::string data);
    TokenType get_type() const;
    std::string get_data() const;
    std::string to_string() const;
    static std::string type_string(TokenType type);
private:
    TokenType type;
    std::string data;
};

#endif
