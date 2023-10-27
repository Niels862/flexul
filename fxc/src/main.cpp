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
    parser.parse();
    
    return 0;
}
