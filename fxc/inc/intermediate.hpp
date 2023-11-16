#ifndef FLEXUL_INTERMEDIATE_HPP
#define FLEXUL_INTERMEDIATE_HPP

#include "opcodes.hpp"
#include "tree.hpp"
#include <vector>
#include <string>
#include <unordered_map>
#include <cstdint>
#include <fstream>

using LabelMap = std::unordered_map<std::string, uint32_t>;

struct Address {
    uint32_t address;
    bool relative;
};

using AddressMap = std::unordered_map<std::string, Address>;

class Node;

class Instruction {
public:
    Instruction();
    Instruction(OpCode opcode, uint32_t operand = 0);
    Instruction(OpCode opcode, FuncCode funccode, uint32_t operand = 0);
    uint32_t assemble(LabelMap const &label_map) const;
    void set_operand(uint32_t operand);
private:
    OpCode opcode;
    FuncCode funccode;
    uint32_t operand;
    std::string label;
    std::string operand_ref;
};

class Intermediate {
public:
    Intermediate();
    void assemble(std::ofstream &file);
    void add_instr(Instruction const &instr);
    Address get_addr(std::string const &var) const;
    void set_addr(std::string const &var, Address addr);
    void print_addr_map() const;
private:
    std::vector<Instruction> stack;
    AddressMap addr_map;
};

#endif
