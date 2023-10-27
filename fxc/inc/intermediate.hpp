#include "opcodes.hpp"
#include <vector>
#include <string>
#include <map>
#include <cstdint>
#include <fstream>

#ifndef FLEXUL_INTERMEDIATE_HPP
#define FLEXUL_INTERMEDIATE_HPP

using LabelMap = std::map<std::string, uint32_t>;

class Instruction {
public:
    Instruction();
    Instruction(OpCode opcode, uint32_t operand = 0);
    Instruction(OpCode opcode, FuncCode funccode, uint32_t operand = 0);
    uint32_t assemble(LabelMap const &label_map) const;
    void set_operand(uint32_t operand);
    void set_label_ref(std::string const &label);
private:
    OpCode opcode;
    FuncCode funccode;
    uint32_t operand;
    std::string label;
};

class Intermediate {
public:
    Intermediate();
    void assemble(std::ofstream &file);
    void add_instr(Instruction const &instr);
private:
    std::vector<Instruction> stack;  // todo attach label to instructions
};

#endif
