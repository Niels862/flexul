#include "mnemonics.hpp"

std::string const op_names[] = {
    "nop", "syscall", "unary", "binary", "push", "sethi", "call", "setframe",
    "ret"
};

std::string const unary_func_names[] = {
    "nop", "neg"
};

std::string const binary_func_names[] = {
    "nop", "add", "sub", "mul", "div", "mod"
};

std::string const syscall_func_names[] = {
    "nop", "exit"
};
