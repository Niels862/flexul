#ifndef FLEXUL_OPCODES_HPP
#define FLEXUL_OPCODES_HPP

enum class OpCode {
    Nop = 0,
    SysCall,
    Unary,
    Binary,
    Push,
    Pop,
    AddSp,
    LoadRel,
    LoadAbs,
    LoadAddrRel,
    DupLoad,
    Dup,
    Call,
    Ret,
    Jump,
    BrTrue,
    BrFalse
};

enum class FuncCode {
    Nop = 0, // binary
    Add,
    Sub,
    Mul,
    Div,
    Mod,
    Equals,
    NotEquals,
    LessThan,
    LessEquals,
    Assign,

    Neg = 1, // unary

    Exit = 1, // syscall
    PutC,
    GetC
};

#endif
