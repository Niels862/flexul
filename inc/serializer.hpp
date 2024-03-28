#ifndef FLEXUL_SERIALIZER_HPP
#define FLEXUL_SERIALIZER_HPP

#include "tree.hpp"
#include "opcodes.hpp"
#include "symbol.hpp"
#include "callable.hpp"
#include <vector>
#include <queue>
#include <stack>
#include <string>
#include <algorithm>
#include <unordered_map>
#include <cstdint>
#include <fstream>

class BaseNode;

class CallableNode;

class Serializer;

using Label = uint32_t;

using LabelMap = std::unordered_map<Label, uint32_t>;

struct JobEntry {
    JobEntry();
    JobEntry(uint32_t label, BaseNode *node, bool no_serialize);

    uint32_t label;
    BaseNode *node;
    bool no_serialize;
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

    size_t get_size() const;

    void disassemble() const;
private:
    EntryType m_type;
    OpCode m_opcode;
    FuncCode m_funccode;
    uint32_t m_data;
    bool m_has_immediate;
    bool m_references_label;
    size_t m_size;
};

class Serializer {
public:
    Serializer();

    SymbolId declare_callable(std::string const &name, SymbolMap &scope, 
            CallableNode *node);

    void call(SymbolId id, 
            std::vector<std::unique_ptr<ExpressionNode>> const &args);
    void push_callable_addr(SymbolId id);

    void add_instr(OpCode opcode, FuncCode funccode = FuncCode::Nop);
    void add_instr(OpCode opcode, uint32_t data, bool references_label = false);
    void add_instr(OpCode opcode, FuncCode funccode, 
            uint32_t data, bool references_label = false);
    void add_job(Label label, BaseNode *node, bool no_serialize);
    void add_function_implementation(SymbolId id);
    uint32_t add_label();
    uint32_t add_label(Label label);
    uint32_t get_label();
    uint32_t get_stack_size() const;

    void serialize(std::unique_ptr<BaseNode> &root);
    std::vector<uint32_t> assemble();
    void disassemble() const;

    SymbolTable &symbol_table();
    InlineFrames &inline_frames();
private:
    void add_entry(StackEntry const &entry);

    SymbolTable m_symbol_table;
    InlineFrames m_inline_frames;

    std::queue<JobEntry> m_code_jobs;
    LabelMap m_labels;
    // First used to give each symbol a unique id, then to attach labels
    // Symbol ids are also used as labels
    // 0 -> invalid/unset id
    // 1 -> entry point id
    uint32_t m_counter;
    std::vector<StackEntry> m_stack;
    CallableMap m_callable_map;
};

#endif
