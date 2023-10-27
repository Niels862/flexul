#include "mnemonics.hpp"

std::string const op_names[] = {
    "nop", "syscall", "alu", "push", "sethi"
};

std::string const binary_func_names[] = {
    "nop", "add", "sub", "mul", "div"
};

std::string const syscall_func_names[] = {
    "nop", "exit"
};
