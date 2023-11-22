#include "serializer.hpp"
#include <iostream>
#include <stdexcept>

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

StackEntry StackEntry::label(uint32_t label) {
    return StackEntry(
            EntryType::Label, OpCode::Nop, FuncCode::Nop, 
            label, false, false);
}

bool StackEntry::combine(StackEntry const &right, StackEntry &combined) const {
    if (type != EntryType::Instruction 
            || right.type != EntryType::Instruction) {
        return false;
    }
    if (opcode == OpCode::Nop) {
        combined = right;
        return true;
    }
    if (right.opcode == OpCode::Nop) {
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
            {0, StorageType::Invalid, 0}, 
            {1, StorageType::Absolute, 0}
        }), counter(2), stack() {}

uint32_t Serializer::get_symbol_id() {
    uint32_t symbol_id = counter;
    counter++;
    return symbol_id;
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

SymbolEntry const &Serializer::get_symbol_entry(uint32_t symbol_id) {
    return symbol_table[symbol_id];
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

void Serializer::add_data_node(uint32_t label, BaseNode *node) {
    data_section.push_back({label, node});
}

uint32_t Serializer::add_label() {
    return add_label(get_label());
}

uint32_t Serializer::add_label(uint32_t label) {
    add_entry(StackEntry::label(label));
    return label;
}

uint32_t Serializer::get_label() {
    uint32_t label = counter;
    counter++;
    return label;
}

void Serializer::serialize(BaseNode *root) {
    std::vector<std::string> symbol_table; // maps from int_id to identifier
    SymbolMap global_symbol_map;
    uint32_t entry_label;
    uint32_t i;

    root->resolve_symbols_first_pass(*this, global_symbol_map);
    root->resolve_symbols_second_pass(*this, global_symbol_map);
    // Note that 'main' may not be a function name but could be another
    // global scope definition, this is intended behavior.
    auto iter = global_symbol_map.find("main"); // TODO: variable entry point
    if (iter == global_symbol_map.end()) {
        throw std::runtime_error("Entry point 'main' was not defined");
    }
    entry_label = iter->second;

    add_instr(OpCode::Push, 0);
    add_instr(OpCode::Push, entry_label, true);
    add_instr(OpCode::Call);
    add_instr(OpCode::SysCall, FuncCode::Exit);
    root->serialize(*this);

    // Data section may still grow during serialization because of lambdas
    for (i = 0; i < data_section.size(); i++) {
        add_label(data_section[i].label);
        data_section[i].node->serialize(*this);
    }
}

void Serializer::assemble(std::ofstream &file) const {
    std::vector<uint32_t> bytecode;
    LabelMap map;
    uint32_t i = 0;
    for (StackEntry const &entry : stack) {
        entry.register_label(map, i);
    }
    for (StackEntry const &entry : stack) {
        entry.assemble(bytecode, map);
    }
    for (uint32_t const bytecode_entry : bytecode) {
        file.write(reinterpret_cast<const char*>(&bytecode_entry), 4);
    }
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
