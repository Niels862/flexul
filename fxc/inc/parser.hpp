#include "tokenizer.hpp"
#include "tree.hpp"
#include <fstream>
#include <unordered_set>

#ifndef FLEXUL_PARSER_HPP
#define FLEXUL_PARSER_HPP

class Parser {
public:
    Parser();
    Parser(std::ifstream &file);
    ~Parser();
    void reset();
    Node *parse();
private:
    Node *new_leaf(Token token);
    Node *new_unary(Token token, Node *first);
    Node *new_binary(Token token, Node *first, Node *second);
    Node *new_ternary(Token token, Node *first, Node *second, Node *third);
    Node *new_n_ary(Token token, std::vector<Node *> children);
    void adopt(Node *node);
    Token get_token();
    Token expect_data(std::string const &data);
    Token expect_type(TokenType type);

    Node *parse_filebody();
    Node *parse_function_declaration();
    Node *parse_statement();
    Node *parse_expression();
    Node *parse_sum();
    Node *parse_term();
    Node *parse_value();
    
    std::unordered_set<Node *> trees;
    Tokenizer tokenizer;
    Token curr_token;
};

#endif
