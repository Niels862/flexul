#include "serializer.hpp"
#include "utils.hpp"
#include <iostream>
#include <stdexcept>
#include <unordered_set>

StackEntry::StackEntry()
        : type(EntryType::Instruction), opcode(OpCode::Nop), 
        funccode(FuncCode::Nop), data(0), has_immediate(0),
        references_label(0), size(0) {}

StackEntry::StackEntry(EntryType type, OpCode opcode, FuncCode funccode, 
        uint32_t data, bool has_immediate, bool references_label)
        : type(type), opcode(opcode), funccode(funccode), 
        data(data), has_immediate(has_immediate), 
        references_label(references_label), size(0) {
    if (type == EntryType::Instruction) {
        size = 1 + has_immediate;
    } else if (type == EntryType::Data) {
        throw std::runtime_error("data not implemented");
    }
}

StackEntry StackEntry::instr(OpCode opcode, FuncCode funccode) {
    return StackEntry(
            EntryType::Instruction, opcode, funccode, 
            0, false, false);
}

StackEntry StackEntry::instr(OpCode opcode, uint32_t data, 
        bool references_label) {
    return StackEntry(
            EntryType::Instruction, opcode, FuncCode::Nop, 
            data, true, references_label);
}
StackEntry StackEntry::instr(OpCode opcode, FuncCode funccode, 
        uint32_t data, bool references_label) {
    return StackEntry(
            EntryType::Instruction, opcode, funccode, 
            data, true, references_label);
}

StackEntry StackEntry::label(Label label) {
    return StackEntry(
            EntryType::Label, OpCode::Nop, FuncCode::Nop, 
            label, false, false);
}

bool StackEntry::has_no_effect() const {
    if (opcode == OpCode::Nop) {
        return true;
    }
    if (opcode == OpCode::AddSp && has_immediate && data == 0) {
        return true;
    }
    return false;
}

bool StackEntry::combine(StackEntry const &right, StackEntry &combined) const {
    if (type != EntryType::Instruction 
            || right.type != EntryType::Instruction) {
        return false;
    }
    if (has_no_effect()) {
        combined = right;
        return true;
    }
    if (right.has_no_effect()) {
        combined = *this;
        return true;
    }
    if (opcode == OpCode::Jump || opcode == OpCode::Ret) {
        combined = *this;
        return true; 
        // TODO: After serializing, remove unused labels and check 
        // if jump label; label: can be combined
    }
    if (opcode == OpCode::Push && has_immediate && !references_label
            && right.has_immediate 
            && (right.opcode == OpCode::BrTrue 
                || right.opcode == OpCode::BrFalse)) {
        if ((data && right.opcode == OpCode::BrTrue) 
                || (!data && right.opcode == OpCode::BrFalse)) {
            combined = StackEntry::instr(
                    OpCode::Jump, right.data, right.references_label);
        } else {
            combined = StackEntry::instr(OpCode::Nop);
        }
        return true;
    }
    if (opcode == OpCode::Push && has_immediate && !right.has_immediate) {
        combined = StackEntry::instr(right.opcode, right.funccode, 
                data, references_label);
        return true;
    }
    return false;
}

void StackEntry::register_label(LabelMap &map, uint32_t &i) const {
    if (type == EntryType::Label) {
        if (map.find(data) != map.end()) {
            throw std::runtime_error(
                    "Redefinition of label " + std::to_string(data));
        }
        map[data] = i;
    }
    i += size;
}

void StackEntry::assemble(std::vector<uint32_t> &stack, 
        LabelMap const &map) const {
    uint32_t immediate = data;
    if (type == EntryType::Invalid) {
        throw std::runtime_error("Invalid entry");
    }
    if (references_label) {
        auto iter = map.find(immediate);
        if (iter == map.end()) {
            throw std::runtime_error(
                    "Unresolved label: " + std::to_string(immediate));
        }
        immediate = iter->second;
    }
    if (type == EntryType::Instruction) {
        stack.push_back(static_cast<uint32_t>(opcode) 
                | (static_cast<uint32_t>(funccode) << 8)
                | (has_immediate << 7));
        if (has_immediate) {
            stack.push_back(immediate);
        }
    }
}

Serializer::Serializer()
        : symbol_table({
            {"<null>", 0, StorageType::Invalid, 0, 0}, 
            {"<entry>", 1, StorageType::Absolute, 0, 0}
        }), counter(2), stack() {}

SymbolId Serializer::get_symbol_id() {
    SymbolId id = counter;
    counter++;
    return id;
}

void Serializer::register_symbol(SymbolEntry const &entry) {
    if (entry.id != symbol_table.size()) {
        throw std::runtime_error(
                "Registered symbol ID does not match expected value: got "
                + std::to_string(entry.id) + ", expected "
                + std::to_string(symbol_table.size()));
    }
    symbol_table.push_back(entry);
}

SymbolEntry const &Serializer::get_symbol_entry(SymbolId id) {
    SymbolId i = id;
    std::unordered_set<SymbolId> aliases;
    SymbolEntry entry = symbol_table[id];

    while (entry.storage_type == StorageType::Alias) {
        if (aliases.find(entry.value) != aliases.end()) {
            throw std::runtime_error(
                    "Circular alias definition: " + symbol_table[id].symbol);
        }
        i = entry.value;
        entry = symbol_table[i];
    }

    symbol_table[i].usages++;
    return symbol_table[i];
}

SymbolId Serializer::declare_symbol(std::string const &symbol, 
        SymbolMap &scope, StorageType storage_type, uint32_t value) {
    SymbolMap::const_iterator iter = scope.find(symbol);
    if (iter != scope.end()) {
        throw std::runtime_error("Redeclared symbol: " + symbol);
    }
    SymbolId id = get_symbol_id();
    scope[symbol] = id;
    register_symbol({symbol, id, storage_type, value, 0});
    return id;
}

void Serializer::open_container() {
    containers.push(std::vector<SymbolId>());
}

void Serializer::add_to_container(SymbolId id) {
    containers.top().push_back(id);
}

uint32_t Serializer::resolve_container() {
    uint32_t position = 0;
    for (SymbolId const &id : containers.top()) {
        symbol_table[id].value = position;
        position++;
    }
    containers.pop();
    return position;
}

void Serializer::dump_symbol_table() const {
    uint32_t i;
    for (i = 0; i < symbol_table.size(); i++) {
        SymbolEntry entry = symbol_table[i];
        std::cerr << std::setw(6) << i << ": " 
                << entry.symbol << " of type " 
                << static_cast<int>(entry.storage_type)
                << " with value " << static_cast<int32_t>(entry.value) 
                << " (" << entry.usages << " usages)" << std::endl;
    }
}

void Serializer::add_instr(OpCode opcode, FuncCode funccode) {
    add_entry(
            StackEntry::instr(opcode, funccode));
}

void Serializer::add_instr(OpCode opcode, uint32_t data, 
        bool references_label) {
    add_entry(
            StackEntry::instr(opcode, data, references_label));
}

void Serializer::add_instr(OpCode opcode, FuncCode funccode, 
        uint32_t data, bool references_label) {
    add_entry(
            StackEntry::instr(opcode, funccode, data, references_label));
}

void Serializer::add_job(uint32_t label, BaseNode *node) {
    code_jobs.push_back({label, node});
}

uint32_t Serializer::add_label() {
    return add_label(get_label());
}

uint32_t Serializer::add_label(Label label) {
    add_entry(StackEntry::label(label));
    return label;
}

uint32_t Serializer::get_label() {
    Label label = counter;
    counter++;
    return label;
}

void Serializer::serialize(BaseNode *root) {
    std::vector<std::string> symbol_table; // maps from int_id to identifier
    SymbolMap global_scope;
    SymbolMap enclosing_scope;
    SymbolMap current_scope;
    Label entry_label;
    uint32_t i;

    root->resolve_symbols_first_pass(*this, global_scope);
    root->resolve_symbols_second_pass(*this, global_scope, 
            enclosing_scope, current_scope);

    // Note that 'main' may not be a function name but could be another
    // global scope definition, this is intended behavior.
    auto iter = global_scope.find("main"); // TODO: variable entry point
    if (iter == global_scope.end()) {
        throw std::runtime_error("Entry point 'main' was not defined");
    }
    entry_label = iter->second;

    add_instr(OpCode::Push, 0);
    add_instr(OpCode::Push, entry_label, true);
    add_instr(OpCode::Call);
    add_instr(OpCode::SysCall, FuncCode::Exit);
    root->serialize(*this);

    for (i = 0; i < code_jobs.size(); i++) {
        add_label(code_jobs[i].label);
        code_jobs[i].node->serialize(*this);
    }
}

std::vector<uint32_t> Serializer::assemble() const {
    std::vector<uint32_t> bytecode;
    LabelMap map;
    uint32_t i = 0;
    for (StackEntry const &entry : stack) {
        entry.register_label(map, i);
    }
    for (StackEntry const &entry : stack) {
        entry.assemble(bytecode, map);
    }
    return bytecode;
}

void Serializer::add_entry(StackEntry const &entry) {
    stack.push_back(entry);
    StackEntry left;
    StackEntry right;
    StackEntry combined;
    while (stack.size() > 1) {
        left = stack[stack.size() - 2];
        right = stack[stack.size() - 1];
        if (left.combine(right, combined)) {
            stack.pop_back();
            stack[stack.size() - 1] = combined;
        } else {
            return;
        }
    }
}
