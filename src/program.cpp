#include "program.hpp"
#include "mnemonics.hpp"
#include "opcodes.hpp"
#include <iostream>
#include <iomanip>

Program::Program() 
        : stack(nullptr), ip(0), bp(0), completed_instrs(0), execution_time(0) {}

Program::~Program() {
    delete[] stack;
}

Program Program::load(std::vector<uint32_t> const &bytecode) {
    Program program;
    program.stack = new uint32_t[10000];
    std::copy(bytecode.data(), bytecode.data() + bytecode.size(), 
            program.stack);
    program.sp = bytecode.size();
    return program;
}

uint32_t Program::run() {
    uint32_t instr, addr, ret_val, n_args, ret_bp, operand;
    int32_t a, b, y;
    OpCode opcode;
    FuncCode funccode;
    clock_t start = std::clock();
    completed_instrs = 0;
    while (ip < sp) {
        instr = stack[ip];
        opcode = static_cast<OpCode>(instr & 0x7F);
        funccode = static_cast<FuncCode>((instr >> 8) & 0xFF);
        if ((instr >> 7) & 1) {
            operand = stack[ip + 1];
            ip++;
        } else if (opcode != OpCode::Nop 
                && !(opcode == OpCode::SysCall && funccode == FuncCode::GetC)) {
            operand = stack[sp];
            sp--;
        }
        switch (opcode) {
            case OpCode::Nop: break;
            case OpCode::SysCall:
                switch (funccode) {
                    case FuncCode::Exit:
                        execution_time = std::clock() - start;
                        return operand;
                    case FuncCode::PutC:
                        sp++;
                        stack[sp] = putc(operand, stdout);
                        break;
                    case FuncCode::GetC:
                        sp++;
                        stack[sp] = getc(stdin);
                        break;
                    default: 
                        throw std::runtime_error(
                                "Unrecognized funccode");
                }
                break;
            case OpCode::Unary:
                a = operand;
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
                stack[sp] = y;
                break;
            case OpCode::Binary: 
                a = stack[sp];
                b = operand;
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
                stack[sp] = y;
                break;
            case OpCode::Push:
                sp++;
                stack[sp] = operand;
                break;
            case OpCode::Pop:
                break;
            case OpCode::AddSp:
                sp += static_cast<int32_t>(operand);
                break;
            case OpCode::LoadRel:
                sp++;
                stack[sp] = stack[bp + operand];
                break;
            case OpCode::LoadAbs:
                sp++;
                stack[sp] = stack[operand];
                break;
            case OpCode::LoadAddrRel:
                sp++;
                stack[sp] = bp + operand;
                break;
            case OpCode::Call:
                // Before call: arguments, N arguments and func address pushed
                addr = operand;
                sp++;
                stack[sp] = bp;
                sp++;
                stack[sp] = ip;
                bp = sp + 1;
                ip = addr - 1;
                break;
            case OpCode::Ret:
                n_args = stack[bp - 3];
                ret_bp = stack[bp - 2];
                addr = stack[bp - 1];
                ret_val = operand;
                sp = bp - n_args - 3;
                stack[sp] = ret_val;
                bp = ret_bp;
                ip = addr;
                break;
            case OpCode::Jump:
                ip = operand - 1;
                break;
            case OpCode::BrTrue:
            case OpCode::BrFalse:
                a = stack[sp];
                addr = operand;
                if ((a && opcode == OpCode::BrTrue)
                        || (!a && opcode == OpCode::BrFalse)) {
                    ip = addr - 1;
                }
                sp--;
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
    for (size_t i = 0; i < sp; i++) {
        std::cout << stack[i] << std::endl;
    }
}

void Program::disassemble() const {
    uint32_t i;
    for (i = 0; i < sp; i++) {
        std::cerr << std::setw(6) << i << ": ";
        disassemble_instr(stack[i], i + 1 < sp ? stack[i + 1] : 0, i);
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
