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

void Instruction::set_label_ref(std::string const &label) {
    this->label = label;
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
