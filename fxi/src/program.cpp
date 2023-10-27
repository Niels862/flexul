#include "program.hpp"
#include "mnemonics.hpp"
#include "opcodes.hpp"
#include <iostream>

Program::Program() 
        : ip(0), bp(0) {}

Program Program::load(std::ifstream &file) {
    Program program;
    uint32_t value = 0;
    while (file.read(reinterpret_cast<char *>(&value), 4)) {
        program.stack.push_back(value);
        value = 0;
    }
    return program;
}

int Program::run() {
    uint32_t instr, operand, a, b, y;
    OpCode opcode;
    FuncCode funccode;
    while (ip < stack.size()) {
        instr = stack[ip];
        opcode = static_cast<OpCode>(instr & 0xFF);
        funccode = static_cast<FuncCode>((instr >> 8) & 0xFF);
        operand = instr >> 16;
        switch (opcode) {
            case OpCode::Nop: break;
            case OpCode::SysCall:
                switch (funccode) {
                    case FuncCode::Exit:
                        return stack[stack.size() - 1];
                    default: break;
                }
                break;
            case OpCode::Binary: 
                a = stack[stack.size() - 2];
                b = stack[stack.size() - 1];
                switch (funccode) {
                    case FuncCode::Nop: break;
                    case FuncCode::Add: y = a + b; break;
                    case FuncCode::Sub: y = a - b; break;
                    case FuncCode::Mul: y = a * b; break;
                    case FuncCode::Div: y = a / b; break;
                    default: break;
                }
                stack[stack.size() - 2] = y;
                stack.pop_back();
                break;
            case OpCode::Push: 
                stack.push_back(operand);
                break;
            case OpCode::SetHi:
                stack[stack.size() - 1] |= operand << 16;
                break;
            default: break;
        }
        ip++;
    }
    return 0;
}

void Program::dump_stack() {
    for (uint32_t const &elem : stack) {
        std::cout << elem << std::endl;
    }
}

void Program::disassemble() const {
    for (uint32_t const &elem : stack) {
        disassemble_instr(elem);
    }
}

void Program::disassemble_instr(uint32_t instr) const {
    OpCode opcode = static_cast<OpCode>(instr & 0xFF);
    FuncCode funccode = static_cast<FuncCode>((instr >> 8) & 0xFF);
    uint32_t operand = instr >> 16;
    std::string func_name;
    if (opcode == OpCode::Binary) {
        func_name = binary_func_names[static_cast<size_t>(funccode)];
    } else if (opcode == OpCode::SysCall) {
        func_name = syscall_func_names[static_cast<size_t>(funccode)];
    }
    if (func_name.empty()) {
        std::cout << op_names[static_cast<size_t>(opcode)] 
                << " " << operand << std::endl;
    } else {
        std::cout << op_names[static_cast<size_t>(opcode)] 
                << " " << func_name << " " << operand << std::endl;
    }
}
