#ifndef FLEXUL_SERIALIZER_HPP
#define FLEXUL_SERIALIZER_HPP

#include "tree.hpp"
#include "opcodes.hpp"
#include "symbol.hpp"
#include "callable.hpp"
#include <vector>
#include <stack>
#include <string>
#include <unordered_map>
#include <cstdint>
#include <fstream>

class BaseNode;

class CallableNode;

class Serializer;

using Label = uint32_t;

using LabelMap = std::unordered_map<Label, uint32_t>;

struct JobEntry {
    uint32_t label;
    BaseNode *node;
    bool no_serialize;
};

struct IntrinsicEntry {
    std::string symbol;
    size_t n_args;
    OpCode opcode;
    FuncCode funccode;
};

extern std::vector<IntrinsicEntry> const intrinsics;

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

    size_t get_size() const;

    void disassemble() const;
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
    SymbolId declare_symbol(std::string const &symbol, SymbolMap &scope, 
            StorageType storage_type, uint32_t value = 0, 
            uint32_t size = 1);
    SymbolId declare_callable(std::string const &name, SymbolMap &scope, 
            CallableNode *node);

    // Opens new storage container
    void open_container();
    // Adds id to container
    void add_to_container(SymbolId id);
    uint32_t get_container_size() const;
    // Resolves every symbol in the current container and closes it
    // Returns size of resolved container
    void resolve_local_container();

    void open_inline_call(BaseNode *params);
    void use_inline_param(uint32_t index);
    void close_inline_call();

    void call(SymbolId id, BaseNode *params);
    void push_callable_addr(SymbolId id);

    void dump_symbol_table() const;

    void add_instr(OpCode opcode, FuncCode funccode = FuncCode::Nop);
    void add_instr(OpCode opcode, uint32_t data, bool references_label = false);
    void add_instr(OpCode opcode, FuncCode funccode, 
            uint32_t data, bool references_label = false);
    void add_job(Label label, BaseNode *node, bool no_serialize);
    uint32_t add_label();
    uint32_t add_label(Label label);
    uint32_t get_label();
    uint32_t get_stack_size() const;

    void load_predefined(SymbolMap &symbol_map);

    void serialize(BaseNode *root);
    std::vector<uint32_t> assemble();
    void disassemble() const;
private:
    void add_entry(StackEntry const &entry);
    std::vector<SymbolEntry> symbol_table;
    std::vector<JobEntry> code_jobs;
    LabelMap labels;
    // First used to give each symbol a unique id, then to attach labels
    // Symbol ids are also used as labels
    // 0 -> invalid/unset id
    // 1 -> entry point id
    uint32_t counter;
    std::vector<StackEntry> stack;
    std::stack<Label> break_labels;
    std::stack<std::vector<SymbolId>> containers;
    std::stack<BaseNode *> inline_params;
    CallableMap callables;
};

#endif
