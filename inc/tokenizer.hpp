#ifndef FLEXUL_TOKENIZER_HPP
#define FLEXUL_TOKENIZER_HPP

#include "token.hpp"
#include <vector>
#include <string>

class Tokenizer {
public:
    Tokenizer();
    Tokenizer(std::string const &filename);
    Token get_token();
    bool eof();
private:
    void next_char();
    void cleanup();
    void assert_no_newline() const;
    Token get_identifier();
    Token get_intlit();
    Token get_charlit();
    Token get_operator();
    Token get_separator();

    std::string m_text;
    size_t m_i;// todo all size_ts to std::size_t
    std::size_t m_row;
    std::size_t m_col;
};

bool is_op_char(char c);
bool is_sep_char(char c);

#endif
