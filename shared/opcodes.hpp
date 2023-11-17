#ifndef FLEXUL_OPCODES_HPP
#define FLEXUL_OPCODES_HPP

enum class OpCode {
    Nop = 0,
    SysCall,
    Unary,
    Binary,
    Push,
    Call,
    Ret
};

enum class FuncCode {
    Nop = 0, // binary
    Add,
    Sub,
    Mul,
    Div,
    Mod,

    Neg = 1, // unary

    Exit = 1 // syscall
};

#endif
