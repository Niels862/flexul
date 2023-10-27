#include <fstream>
#include <vector>
#include <cstdint>

#ifndef FLEXUL_PROGRAM_HPP
#define FLEXUL_PROGRAM_HPP

class Program {
public:
    Program();
    static Program load(std::ifstream &file);
    int run();
    void dump_stack();
    void disassemble() const;
    void disassemble_instr(uint32_t instr) const;
private:
    std::vector<uint32_t> stack;
    size_t ip;
    size_t bp;
};

#endif
