#include "program.hpp"
#include "mnemonics.hpp"
#include "opcodes.hpp"
#include <iostream>
#include <iomanip>

Program::Program() 
        : ip(0), bp(0), completed_instrs(0), execution_time(0) {}

Program Program::load(std::vector<uint32_t> bytecode) {
    Program program;
    program.stack = bytecode;
    return program;
}

uint32_t Program::run() {
    uint32_t instr, addr, ret_val, n_args, ret_bp, imm;
    int32_t a, b, y;
    OpCode opcode;
    FuncCode funccode;
    clock_t start = std::clock();
    completed_instrs = 0;
    while (ip < stack.size()) {
        instr = stack[ip];
        opcode = static_cast<OpCode>(instr & 0x7F);
        funccode = static_cast<FuncCode>((instr >> 8) & 0xFF);
        if ((instr >> 7) & 1) {
            imm = stack[ip + 1];
            ip++;
        } else if (opcode != OpCode::Nop) {
            imm = stack[stack.size() - 1];
            stack.pop_back();
        }
        switch (opcode) {
            case OpCode::Nop: break;
            case OpCode::SysCall:
                switch (funccode) {
                    case FuncCode::Exit:
                        execution_time = std::clock() - start;
                        return imm;
                    default: 
                        throw std::runtime_error(
                                "Unrecognized funccode");
                }
                break;
            case OpCode::Unary:
                a = imm;
                y = a;
                switch (funccode) {
                    case FuncCode::Nop:
                        break;
                    case FuncCode::Neg:
                        y = -a;
                        break;
                    default: 
                        throw std::runtime_error(
                                "Unrecognized funccode");
                }
                stack.push_back(y);
                break;
            case OpCode::Binary: 
                a = stack[stack.size() - 1];
                b = imm;
                y = a;
                switch (funccode) {
                    case FuncCode::Nop: 
                        break;
                    case FuncCode::Add:
                        y = a + b; 
                        break;
                    case FuncCode::Sub:
                        y = a - b;
                        break;
                    case FuncCode::Mul:
                        y = a * b;
                        break;
                    case FuncCode::Div:
                        if (b == 0) {
                            throw std::runtime_error("Division by zero");
                        }
                        y = a / b;
                        break;
                    case FuncCode::Mod: 
                        y = a % b;
                        break;
                    case FuncCode::Equals:
                        y = a == b;
                        break;
                    case FuncCode::NotEquals:
                        y = a != b;
                        break;
                    case FuncCode::LessThan:
                        y = a < b;
                        break;
                    case FuncCode::LessEquals:
                        y = a <= b;
                        break;
                    case FuncCode::Assign:
                        stack[a] = b;
                        y = b;
                        break;
                    default: 
                        throw std::runtime_error(
                                "Unrecognized funccode");
                }
                stack[stack.size() - 1] = y;
                break;
            case OpCode::Push:
                stack.push_back(imm);
                break;
            case OpCode::Pop:
                break;
            case OpCode::LoadRel:
                a = imm;
                stack.push_back(stack[bp + a]);
                break;
            case OpCode::LoadAbs:
                a = imm;
                stack.push_back(stack[a]);
                break;
            case OpCode::LoadAddrRel:
                a = imm;
                stack.push_back(bp + a);
                break;
            case OpCode::Call:
                // Before call: arguments, N arguments and func address pushed
                addr = imm;
                stack.push_back(bp);
                stack.push_back(ip);
                bp = stack.size();
                ip = addr - 1;
                break;
            case OpCode::Ret:
                n_args = stack[bp - 3];
                ret_bp = stack[bp - 2];
                addr = stack[bp - 1];
                ret_val = imm;
                stack.resize(bp - 3 - n_args);
                stack.push_back(ret_val);
                bp = ret_bp;
                ip = addr;
                break;
            case OpCode::Jump:
                addr = imm;
                ip = addr - 1;
                break;
            case OpCode::BrTrue:
            case OpCode::BrFalse:
                a = stack[stack.size() - 1];
                addr = imm;
                if ((a && opcode == OpCode::BrTrue)
                        || (!a && opcode == OpCode::BrFalse)) {
                    ip = addr - 1;
                }
                stack.pop_back();
                break;
            default: 
                break;
        }
        completed_instrs++;
        ip++;
    }
    execution_time = std::clock() - start;
    std::cerr << "Instruction fetch overread at " << ip << std::endl;
    return -1;
}

void Program::analytics() const {
    double execution_time_secs = 
            static_cast<double>(execution_time) / CLOCKS_PER_SEC;
    std::cout << "Instructions completed:  " 
            << completed_instrs << std::endl;
    std::cout << "Execution time:          " 
            << execution_time_secs << std::endl;
    std::cout << "Seconds per instruction: " 
            << (execution_time_secs / completed_instrs) << std::endl;
    std::cout << "Instructions per second: " 
            << static_cast<uint64_t>(completed_instrs / execution_time_secs) 
            << std::endl;
}

void Program::dump_stack() const {
    for (uint32_t const &elem : stack) {
        std::cout << elem << std::endl;
    }
}

void Program::disassemble() const {
    uint32_t i;
    for (i = 0; i < stack.size(); i++) {
        std::cerr << std::setw(6) << i << ": ";
        disassemble_instr(stack[i], i + 1 < stack.size() ? stack[i + 1] : 0, i);
    }
}

void Program::disassemble_instr(
        uint32_t instr, uint32_t next, uint32_t &i) const {
    OpCode opcode = static_cast<OpCode>(instr & 0x7F);
    FuncCode funccode = static_cast<FuncCode>((instr >> 8) & 0xFF);
    std::string func_name;
    if (opcode == OpCode::Unary) {
        func_name = unary_func_names[static_cast<size_t>(funccode)];
    } else if (opcode == OpCode::Binary) {
        func_name = binary_func_names[static_cast<size_t>(funccode)];
    } else if (opcode == OpCode::SysCall) {
        func_name = syscall_func_names[static_cast<size_t>(funccode)];
    }
    if (func_name.empty()) {
        std::cerr << op_names[static_cast<size_t>(opcode)];
    } else {
        std::cerr << op_names[static_cast<size_t>(opcode)] << " " << func_name;
    }
    if ((instr >> 7) & 1) {
        if (next >> 31) {
            std::cerr << " " << static_cast<int32_t>(next) 
                    << " (" << next << ")" << std::endl;
        } else {
            std::cerr << " " << next << std::endl;
        }
        i++;
    } else {
        std::cerr << std::endl;
    }
}
