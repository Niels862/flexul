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
    void adopt(Node *node);
    Token get_token();

    Node *parse_filebody();
    Node *parse_statement();
    Node *parse_sum();
    Node *parse_value();
    
    std::unordered_set<Node *> trees;
    Tokenizer tokenizer;
    Token curr_token;
};

#endif
