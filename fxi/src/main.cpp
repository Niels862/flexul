#include "program.hpp"
#include <iostream>
#include <string>

int main(int argc, char *argv[]) {
    if (argc < 2) {
        std::cout << "Expected file name" << std::endl;
        return 1;
    }
    std::ifstream file(argv[1], std::ios::binary);
    if (!file) {
        std::cout << "Failed to open file: " + std::string(argv[1]) 
                << std::endl;
        return 1;
    }
    Program program = Program::load(file);
    uint32_t exit_code = program.run();
    std::cout << "Program finished with exit code " 
            << exit_code << std::endl;
    return 0;
}
