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
    if (argc < 3) {
        outfilename = "out.bin";
    } else {
        outfilename = argv[2];
    }
    
    std::ifstream infile(infilename);
    if (!infile) {
        std::cout << "Failed to open " << infilename << std::endl;
        return 1;
    }
    std::ofstream outfile(outfilename);
    if (!outfile) {
        std::cout << "Failed to open " << outfilename << std::endl;
        return 1;
    }

    Parser parser(infile);
    Intermediate intermediate;
    try {
        Node *node = parser.parse();
        Node::print(node);
        node->translate(intermediate);
        intermediate.assemble(outfile);
        std::cout << "Successfully wrote to " << outfilename << std::endl;
    } catch (std::exception const &e) {
        std::cerr << e.what() << std::endl;
        return 1;
    }
    return 0;
}
