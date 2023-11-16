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
    BaseNode *new_intlit(Token token);
    BaseNode* new_variable(Token token);
    BaseNode *new_unary(Token token, BaseNode *first);
    BaseNode *new_binary(Token token, BaseNode *first, BaseNode *second);
    BaseNode *new_block(std::vector<BaseNode *> children);
    void adopt(BaseNode *BaseNode);
    Token get_token();
    Token expect_data(std::string const &data);
    Token expect_type(TokenType type);

    BaseNode *parse_filebody();
    // BaseNode *parse_function_declaration();
    // BaseNode *parse_argslist(bool declaration);
    BaseNode *parse_statement();
    BaseNode *parse_expression();
    BaseNode *parse_sum();
    BaseNode *parse_term();
    BaseNode *parse_value();
    
    std::unordered_set<BaseNode *> trees;
    Tokenizer tokenizer;
    Token curr_token;
};

#endif
