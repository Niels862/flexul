#ifndef FLEXUL_MNEMONICS_HPP
#define FLEXUL_MNEMONICS_HPP

#include "opcodes.hpp"
#include <string>

extern std::string const op_names[];

extern std::string const binary_func_names[];

extern std::string const unary_func_names[];

extern std::string const syscall_func_names[];

std::string const &get_op_name(OpCode opcode);

std::string const &get_func_name(OpCode opcode, FuncCode funccode);

#endif
