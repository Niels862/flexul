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

using Label = uint32_t;

using LabelMap = std::unordered_map<Label, uint32_t>;

struct JobEntry {
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
    static StackEntry label(Label label);

    bool has_no_effect() const;
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

    SymbolId get_symbol_id();
    void register_symbol(SymbolEntry const &entry);
    SymbolEntry const &get_symbol_entry(SymbolId id);

    // Opens new storage container
    void open_container();
    // Adds id to container
    void add_to_container(SymbolId id);
    // Resolves every symbol in the current container and closes it
    // Returns size of resolved container
    uint32_t resolve_container();

    void dump_symbol_table() const;

    void add_instr(OpCode opcode, FuncCode funccode = FuncCode::Nop);
    void add_instr(OpCode opcode, uint32_t data, bool references_label = false);
    void add_instr(OpCode opcode, FuncCode funccode, 
            uint32_t data, bool references_label = false);
    void add_job(Label label, BaseNode *node);
    uint32_t add_label();
    uint32_t add_label(Label label);
    uint32_t get_label();

    void serialize(BaseNode *root);
    std::vector<uint32_t> assemble() const;
private:
    void add_entry(StackEntry const &entry);
    std::vector<SymbolEntry> symbol_table;
    std::vector<JobEntry> code_jobs;
    // First used to give each symbol a unique id, then to attach labels
    // Symbol ids are also used as labels
    // 0 -> invalid/unset id
    // 1 -> entry point id
    uint32_t counter;
    std::vector<StackEntry> stack;
    std::stack<Label> break_labels;
    std::stack<std::vector<SymbolId>> containers;
};

#endif