#ifndef FLEXUL_TOKENIZER_HPP
#define FLEXUL_TOKENIZER_HPP

#include "token.hpp"
#include <vector>
#include <string>

class Tokenizer {
public:
    Tokenizer();
    Tokenizer(std::string const &text);
    Token get_token();
    bool eof();
private:
    void cleanup();
    Token get_identifier();
    Token get_intlit();
    Token get_operator();
    Token get_separator();

    std::string text;
    size_t i;
};

bool is_op_char(char c);
bool is_sep_char(char c);

#endif
