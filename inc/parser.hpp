#ifndef FLEXUL_PARSER_HPP
#define FLEXUL_PARSER_HPP

#include "tokenizer.hpp"
#include "tree.hpp"
#include <fstream>
#include <unordered_set>
#include <stack>

class Parser {
public:
    Parser();
    Parser(std::string const &filename);
    std::unique_ptr<BaseNode> parse();
private:
    // Overrides curr_token
    void include_file(std::string const &filename);

    Token get_token();
    
    Token expect_data(std::string const &data);
    Token expect_type(TokenType type);
    Token expect_token(Token const &other);
    Token accept_data(std::string const &data);
    Token accept_type(TokenType type);
    Token check_data(std::string const &data) const;
    Token check_type(TokenType type) const;

    std::unique_ptr<BaseNode> parse_filebody();
    void parse_include();
    std::unique_ptr<BaseNode> parse_function_declaration();
    std::unique_ptr<BaseNode> parse_inline_declaration();
    std::unique_ptr<ExpressionListNode> parse_param_list();
    CallableSignature parse_param_declaration();
    std::unique_ptr<BaseNode> parse_braced_block(bool is_scope);
    std::unique_ptr<BaseNode> parse_type_declaration();
    std::unique_ptr<TypeNode> parse_type();
    std::unique_ptr<BaseNode> parse_statement();
    std::unique_ptr<BaseNode> parse_if_else();
    std::unique_ptr<BaseNode> parse_for();
    std::unique_ptr<BaseNode> parse_while();
    std::unique_ptr<BaseNode> parse_var_declaration();
    std::unique_ptr<BaseNode> parse_expression();
    std::unique_ptr<BaseNode> parse_assignment();
    std::unique_ptr<BaseNode> parse_lambda();
    std::unique_ptr<BaseNode> parse_ternary();
    std::unique_ptr<BaseNode> parse_or();
    std::unique_ptr<BaseNode> parse_and();
    std::unique_ptr<BaseNode> parse_equality_1();
    std::unique_ptr<BaseNode> parse_equality_2();
    std::unique_ptr<BaseNode> parse_sum();
    std::unique_ptr<BaseNode> parse_term();
    std::unique_ptr<BaseNode> parse_value();
    std::unique_ptr<BaseNode> parse_postfix(std::unique_ptr<BaseNode> value);
    
    std::stack<Tokenizer> m_tokenizers;
    Token m_curr_token;
    std::unordered_set<std::string> m_included_files;
};

#endif
