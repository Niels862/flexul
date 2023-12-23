#ifndef FLEXUL_TOKENIZER_HPP
#define FLEXUL_TOKENIZER_HPP

#include "token.hpp"
#include <vector>
#include <string>
#include <unordered_map>

using SyntaxMap = std::unordered_map<std::string, TokenType>;

class Tokenizer {
public:
    Tokenizer();
    Tokenizer(std::string const &text);
    Token get_token();
    bool eof();
private:
    void cleanup();
    void assert_no_newline() const;
    Token get_identifier();
    Token get_intlit();
    Token get_charlit();
    Token get_operator();
    Token get_separator();

    SyntaxMap syntax_map;
    std::string text;
    size_t i;
};

bool is_op_char(char c);
bool is_sep_char(char c);

#endif
