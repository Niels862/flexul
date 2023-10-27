#include "parser.hpp"
#include <iostream>

Parser::Parser() {
}

Parser::Parser(std::ifstream &file) {
    std::string text;
    text.assign(
        (std::istreambuf_iterator<char>(file)),
        (std::istreambuf_iterator<char>())
    );
    tokenizer = Tokenizer(text);
}

Tree Parser::parse() {
    while (!tokenizer.eof()) {
        std::cout << tokenizer.get_token().to_string() << std::endl;
    }

    return tree;
}
