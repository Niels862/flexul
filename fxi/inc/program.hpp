#ifndef FLEXUL_PROGRAM_HPP
#define FLEXUL_PROGRAM_HPP

#include <fstream>
#include <vector>
#include <cstdint>

class Program {
public:
    Program();
    static Program load(std::ifstream &file);
    uint32_t run();
    void dump_stack();
    void disassemble() const;
    void disassemble_instr(uint32_t instr) const;
private:
    std::vector<uint32_t> stack;
    uint32_t ip;
    uint32_t bp;
};

#endif
