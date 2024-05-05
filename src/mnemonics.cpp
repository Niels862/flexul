#include "mnemonics.hpp"

std::string empty = "";

std::string const op_names[] = {
    "nop", "syscall", "unary", "binary", 
    "push", "pop", "addsp", "loadrel", "loadabs", "loadaddrrel", "dupload",
    "dup", "call", "ret", "jump", "brtrue", "brfalse"
};

std::string const unary_func_names[] = {
    "nop", "neg"
};

std::string const binary_func_names[] = {
    "nop", "add", "sub", "mul", "div", "mod", "equal", "notequal", 
    "lessthan", "lessequal", "assign"
};

std::string const syscall_func_names[] = {
    "nop", "exit", "putc", "getc"
};

std::string const &get_op_name(OpCode opcode) {
    return op_names[static_cast<size_t>(opcode)];
}

std::string const &get_func_name(OpCode opcode, FuncCode funccode) {
    switch (opcode) {
        case OpCode::Unary:
            return unary_func_names[static_cast<size_t>(funccode)];
        case OpCode::Binary:
            return binary_func_names[static_cast<size_t>(funccode)];
        case OpCode::SysCall:
            return syscall_func_names[static_cast<size_t>(funccode)];
        default:
            return empty;
    }
}
