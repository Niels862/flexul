#include "parser.hpp"
#include <iostream>
#include <fstream>

int main(int argc, char *argv[]) {
    std::string infilename, outfilename;
    if (argc < 2) {
        infilename = "in.fx";
    } else {
        infilename = argv[1];
    }
    
    std::ifstream infile(infilename);
    if (!infile) {
        std::cout << "Failed to open " << infilename << std::endl;
        return 1;
    }
    Parser parser(infile);
    try {
        Node *node = parser.parse();
        Node::print(node);
    } catch (std::exception const &e) {
        std::cerr << e.what() << std::endl;
        return 1;
    }
    return 0;
}
