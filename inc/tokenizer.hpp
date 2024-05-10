#ifndef FLEXUL_TOKENIZER_HPP
#define FLEXUL_TOKENIZER_HPP

#include "token.hpp"
#include "utils.hpp"
#include <vector>
#include <string>

class Tokenizer {
public:
    Tokenizer();
    Tokenizer(std::string const &filename);
    ~Tokenizer() {
        print_iterable(m_tokens);
    }
    Token get_token();
    bool eof();

    TokenList const &list() const { return m_tokens; }
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

    TokenList m_tokens;
};

bool is_op_char(char c);
bool is_sep_char(char c);

#endif
