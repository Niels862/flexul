#include "intermediate.hpp"
#include <iostream>

Instruction::Instruction()
        : opcode(OpCode::Nop), funccode(FuncCode::Nop), operand(0) {}

Instruction::Instruction(OpCode opcode, uint32_t operand) 
        : opcode(opcode), funccode(FuncCode::Nop), operand(operand) {}

Instruction::Instruction(OpCode opcode, FuncCode funccode, uint32_t operand)
        : opcode(opcode), funccode(funccode), operand(operand) {}

uint32_t Instruction::assemble(LabelMap const &label_map) const {
    uint32_t operand = this->operand;
    if (!this->label.empty()) {
        LabelMap::const_iterator pos = label_map.find(this->label);
        if (pos == label_map.end()) {
            throw std::runtime_error("Unresolved label: " + label);
        } else {
            operand = pos->second;
        }
    }
    return static_cast<uint32_t>(opcode) 
            | (static_cast<uint32_t>(funccode) << 8)
            | (static_cast<uint32_t>(operand) << 16);
}

void Instruction::set_operand(uint32_t operand) {
    this->operand = operand;
}

Intermediate::Intermediate() {}

void Intermediate::assemble(std::ofstream &file) {
    uint32_t value;
    LabelMap label_map;
    for (Instruction const &elem : stack) {
        value = elem.assemble(label_map);
        std::cout << value << std::endl;
        file.write(reinterpret_cast<const char*>(&value), 4);
    }
}

void Intermediate::add_instr(Instruction const &instr) {
    stack.push_back(instr);
}

Address Intermediate::get_addr(std::string const &var) const {
    AddressMap::const_iterator iter = addr_map.find(var);
    if (iter == addr_map.end()) {
        throw std::runtime_error("No address entry for " + var);
    }
    return iter->second;
}

void Intermediate::set_addr(std::string const &var, Address addr) {
    addr_map[var] = addr;
}

void Intermediate::print_addr_map() const {
    for (auto const &addr_key_value : addr_map) {
        std::cout << addr_key_value.first << " --> " 
                << static_cast<int32_t>(addr_key_value.second.address) 
                << " local: " << addr_key_value.second.relative << std::endl;
    }
}
