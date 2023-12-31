#include "mnemonics.hpp"

std::string const op_names[] = {
    "NOP", "SYSCALL", "UNARY", "BINARY", 
    "PUSH", "POP", "ADDSP", "LOADREL", "LOADABS", "LOADADDRREL",
    "CALL", "RET", "JUMP", "BRTRUE", "BRFALSE"
};

std::string const unary_func_names[] = {
    "NOP", "NEG"
};

std::string const binary_func_names[] = {
    "NOP", "ADD", "SUB", "MUL", "DIV", "MOD", "EQUAL", "NOTEQUAL", 
    "LESSTHAN", "GREATERTHAN", "ASSIGN"
};

std::string const syscall_func_names[] = {
    "NOP", "EXIT", "PUTC", "GETC"
};
