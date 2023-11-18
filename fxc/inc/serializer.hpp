#ifndef FLEXUL_SERIALIZER_HPP
#define FLEXUL_SERIALIZER_HPP

#include "tree.hpp"
#include "opcodes.hpp"
#include <vector>
#include <unordered_map>
#include <cstdint>
#include <fstream>

using LabelMap = std::unordered_map<uint32_t, uint32_t>;

class BaseNode;

class StackEntry {
public:
    StackEntry();
    explicit StackEntry(uint32_t data);
    explicit StackEntry(OpCode opcode, FuncCode funccode = FuncCode::Nop);
    void set_label(uint32_t label);
    void register_label(LabelMap &map, uint32_t idx) const;
    uint32_t assemble(LabelMap const &map) const;
private:
    OpCode opcode;
    FuncCode funccode;
    bool data_entry;
    bool label_entry;
    uint32_t label;
    uint32_t data;
};

class Serializer {
public:
    Serializer();
    Serializer &add_data(uint32_t data = 0);
    Serializer &add_instr(OpCode opcode, FuncCode funccode = FuncCode::Nop);
    uint32_t attach_label();
    uint32_t attach_entry_label();
    void references_label(uint32_t label);
    void serialize(BaseNode *root);
    void assemble(std::ofstream &file) const;
private:
    std::vector<StackEntry> stack;
    uint32_t label_counter;
};

#endif