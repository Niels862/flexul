#include "program.hpp"
#include "mnemonics.hpp"
#include "opcodes.hpp"
#include <iostream>
#include <iomanip>
#include "utils.hpp"
Program::Program() 
        : m_ip(0), m_bp(0), m_completed_instrs(0), m_execution_time(0) {}

Program Program::load(std::vector<uint32_t> bytecode) {
    Program program;
    program.m_stack = bytecode;
    return program;
}

uint32_t Program::run() {
    uint32_t instr, addr, ret_val, n_args, ret_bp, operand;
    int32_t a, b, y;
    OpCode opcode;
    FuncCode funccode;
    clock_t start = std::clock();
    m_completed_instrs = 0;
    while (m_ip < m_stack.size()) {
        instr = m_stack[m_ip];
        opcode = static_cast<OpCode>(instr & 0x7F);
        funccode = static_cast<FuncCode>((instr >> 8) & 0xFF);
        if ((instr >> 7) & 1) {
            operand = m_stack[m_ip + 1];
            m_ip++;
        } else if (opcode != OpCode::Nop 
                && !(opcode == OpCode::SysCall && funccode == FuncCode::GetC)) {
            operand = m_stack[m_stack.size() - 1];
            m_stack.pop_back();
        }
        switch (opcode) {
            case OpCode::Nop: break;
            case OpCode::SysCall:
                switch (funccode) {
                    case FuncCode::Exit:
                        m_execution_time = std::clock() - start;
                        return operand;
                    case FuncCode::PutC:
                        m_stack.push_back(putc(operand, stdout));
                        break;
                    case FuncCode::GetC:
                        m_stack.push_back(getc(stdin));
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
                m_stack.push_back(y);
                break;
            case OpCode::Binary: 
                a = m_stack[m_stack.size() - 1];
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
                        m_stack[a] = b;
                        y = b;
                        break;
                    default: 
                        throw std::runtime_error(
                                "Unrecognized funccode");
                }
                m_stack[m_stack.size() - 1] = y;
                break;
            case OpCode::Push:
                m_stack.push_back(operand);
                break;
            case OpCode::Pop:
                break;
            case OpCode::AddSp:
                m_stack.resize(m_stack.size() + static_cast<int32_t>(operand));
                break;
            case OpCode::LoadRel:
                a = operand;
                m_stack.push_back(m_stack[m_bp + a]);
                break;
            case OpCode::LoadAbs:
                a = operand;
                m_stack.push_back(m_stack[a]);
                break;
            case OpCode::LoadAddrRel:
                a = operand;
                m_stack.push_back(m_bp + a);
                break;
            case OpCode::DupLoad:
                m_stack.push_back(operand);
                m_stack.push_back(m_stack[operand]);
                break;
            case OpCode::Dup:
                m_stack.push_back(operand);
                m_stack.push_back(operand);
                break;
            case OpCode::Call:
                // Before call: arguments, N arguments and func address pushed
                addr = operand;
                m_stack.push_back(m_bp);
                m_stack.push_back(m_ip);
                m_bp = m_stack.size();
                m_ip = addr - 1;
                break;
            case OpCode::Ret:
                n_args = m_stack[m_bp - 3];
                ret_bp = m_stack[m_bp - 2];
                addr = m_stack[m_bp - 1];
                ret_val = operand;
                m_stack.resize(m_bp - 3 - n_args);
                m_stack.push_back(ret_val);
                m_bp = ret_bp;
                m_ip = addr;
                break;
            case OpCode::Jump:
                addr = operand;
                m_ip = addr - 1;
                break;
            case OpCode::BrTrue:
            case OpCode::BrFalse:
                a = m_stack[m_stack.size() - 1];
                addr = operand;
                if ((a && opcode == OpCode::BrTrue)
                        || (!a && opcode == OpCode::BrFalse)) {
                    m_ip = addr - 1;
                }
                m_stack.pop_back();
                break;
            default: 
                break;
        }
        m_completed_instrs++;
        m_ip++;
    }
    m_execution_time = std::clock() - start;
    std::cerr << "Instruction fetch overread at " << m_ip << std::endl;
    return -1;
}

void Program::analytics() const {
    double execution_time_secs = 
            static_cast<double>(m_execution_time) / CLOCKS_PER_SEC;
    std::cout << "Instructions completed:  " 
            << m_completed_instrs << std::endl;
    std::cout << "Execution time:          " 
            << execution_time_secs << std::endl;
    std::cout << "Seconds per instruction: " 
            << (execution_time_secs / m_completed_instrs) << std::endl;
    std::cout << "Instructions per second: " 
            << static_cast<uint64_t>(m_completed_instrs / execution_time_secs) 
            << std::endl;
}

void Program::dump_stack() const {
    for (uint32_t const &elem : m_stack) {
        std::cout << elem << std::endl;
    }
}

void Program::disassemble() const {
    uint32_t i;
    for (i = 0; i < m_stack.size(); i++) {
        std::cerr << std::setw(6) << i << ": ";
        disassemble_instr(m_stack[i], i + 1 < m_stack.size() ? m_stack[i + 1] : 0, i);
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
