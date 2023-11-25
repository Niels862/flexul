#ifndef FLEXUL_SERIALIZER_HPP
#define FLEXUL_SERIALIZER_HPP

#include "tree.hpp"
#include "opcodes.hpp"
#include "symbol.hpp"
#include <vector>
#include <stack>
#include <string>
#include <unordered_map>
#include <cstdint>
#include <fstream>

class BaseNode;

using LabelMap = std::unordered_map<uint32_t, uint32_t>;

struct DataEntry {
    uint32_t label;
    BaseNode *node;
};

enum class EntryType {
    Invalid, Instruction, Data, Label
};

class StackEntry {
public:
    StackEntry();
    StackEntry(EntryType type, OpCode opcode, FuncCode funccode, uint32_t data, 
            bool has_immediate, bool references_label);
    static StackEntry instr(OpCode opcode, FuncCode funccode = FuncCode::Nop);
    static StackEntry instr(OpCode opcode, uint32_t data, 
            bool references_label = false);
    static StackEntry instr(OpCode opcode, FuncCode funccode, 
            uint32_t data, bool references_label = false);
    static StackEntry label(uint32_t label);

    bool combine(StackEntry const &right, StackEntry &combined) const;
    void register_label(LabelMap &map, uint32_t &i) const;
    void assemble(std::vector<uint32_t> &stack, LabelMap const &map) const;
private:
    EntryType type;
    OpCode opcode;
    FuncCode funccode;
    uint32_t data;
    bool has_immediate;
    bool references_label;
    size_t size;
};

class Serializer {
public:
    Serializer();

    uint32_t get_symbol_id();
    void register_symbol(SymbolEntry const &entry);
    SymbolEntry const &get_symbol_entry(uint32_t symbol_id);

    void add_instr(OpCode opcode, FuncCode funccode = FuncCode::Nop);
    void add_instr(OpCode opcode, uint32_t data, bool references_label = false);
    void add_instr(OpCode opcode, FuncCode funccode, 
            uint32_t data, bool references_label = false);
    void add_data_node(uint32_t label, BaseNode *node);
    uint32_t add_label();
    uint32_t add_label(uint32_t label);
    uint32_t get_label();
    void references_label(uint32_t label);
    void serialize(BaseNode *root);
    void assemble(std::ofstream &file) const;
private:
    void add_entry(StackEntry const &entry);
    std::vector<SymbolEntry> symbol_table;
    std::vector<DataEntry> data_section;
    // First used to give each symbol a unique id, then to attach labels
    // Symbol ids are also used as labels
    // 0 -> invalid/unset id
    // 1 -> entry point id
    uint32_t counter;
    std::vector<StackEntry> stack;
    std::stack<uint32_t> break_labels;
};

#endif
