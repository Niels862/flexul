#ifndef FLEXUL_PARSER_HPP
#define FLEXUL_PARSER_HPP

#include "tokenizer.hpp"
#include "tree.hpp"
#include <fstream>
#include <unordered_set>

class Parser {
public:
    Parser();
    Parser(std::ifstream &file);
    ~Parser();
    BaseNode *parse();
private:
    template <typename T>
    T add(T node);
    void adopt(BaseNode *BaseNode);
    Token get_token();
    Token expect_data(std::string const &data);
    Token expect_type(TokenType type);
    Token expect_token(Token const &other);
    Token accept_data(std::string const &data);
    Token accept_type(TokenType type);
    Token check_type(TokenType type) const;

    BaseNode *parse_filebody();
    BaseNode *parse_function_declaration();
    BaseNode *parse_param_list(Token const &end_token);
    std::vector<Token> parse_param_declaration(Token const &end_token);
    BaseNode *parse_braced_block(bool is_scope);
    BaseNode *parse_statement();
    BaseNode *parse_if_else();
    BaseNode *parse_for();
    BaseNode *parse_while();
    BaseNode *parse_var_declaration();
    BaseNode *parse_alias();
    BaseNode *parse_expression();
    BaseNode *parse_assignment();
    BaseNode *parse_lambda();
    BaseNode *parse_ternary();
    BaseNode *parse_or();
    BaseNode *parse_and();
    BaseNode *parse_equality_1();
    BaseNode *parse_equality_2();
    BaseNode *parse_sum();
    BaseNode *parse_term();
    BaseNode *parse_value();
    BaseNode *parse_postfix(BaseNode *value);
    
    std::unordered_set<BaseNode *> trees;
    Tokenizer tokenizer;
    Token curr_token;
};

#endif
