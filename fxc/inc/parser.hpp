#include "tokenizer.hpp"
#include "tree.hpp"
#include <fstream>

#ifndef FLEXUL_PARSER_HPP
#define FLEXUL_PARSER_HPP

class Parser {
public:
    Parser();
    Parser(std::ifstream &file);
    Tree parse();
private:
    Tree tree;
    Tokenizer tokenizer;
};

#endif
