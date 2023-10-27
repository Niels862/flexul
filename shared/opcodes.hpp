#ifndef FLEXUL_OPCODES_HPP
#define FLEXUL_OPCODES_HPP

enum class OpCode {
    Nop = 0,
    SysCall,
    Binary,
    Push,
    SetHi
};

enum class FuncCode {
    Nop = 0,
    Add,
    Sub,
    Mul,
    Div,

    Exit = 1
};



#endif
