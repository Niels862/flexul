#ifndef FLEXUL_PROGRAM_HPP
#define FLEXUL_PROGRAM_HPP

#include <fstream>
#include <vector>
#include <cstdint>
#include <ctime>

class Program {
public:
    Program();
    static Program load(std::ifstream &file);
    uint32_t run();
    void analytics() const;
    void dump_stack() const;
    void disassemble() const;
    void disassemble_instr(uint32_t instr, uint32_t next, uint32_t &i) const;
private:
    std::vector<uint32_t> stack;
    uint32_t ip;
    uint32_t bp;
    
    uint64_t completed_instrs;
    clock_t execution_time;
};

#endif
