#include "serializer.hpp"
#include <iostream>
#include <stdexcept>

StackEntry::StackEntry()
        : opcode(OpCode::Nop), funccode(FuncCode::Nop), data_entry(true),
        label_entry(false), label(0), data(0) {}

StackEntry::StackEntry(uint32_t data)
        : opcode(OpCode::Nop), funccode(FuncCode::Nop), data_entry(true),
        label_entry(false), label(0), data(data) {}

StackEntry::StackEntry(OpCode opcode, FuncCode funccode)
        : opcode(opcode), funccode(funccode), data_entry(false),
        label_entry(false), label(0), data(0) {}

void StackEntry::set_label(uint32_t label) {
    if (label_entry) {
        throw std::runtime_error("Cannot reassign label");
    }
    this->label = label;
    label_entry = true;
}

void StackEntry::register_label(LabelMap &map, uint32_t idx) const {
    if (!data_entry && label_entry) {
        if (map.find(label) != map.end()) {
            throw std::runtime_error(
                    "Redefinition of label " + std::to_string(label));
        }
        map[label] = idx + 1; // label points to next entry
    }
}

uint32_t StackEntry::assemble(LabelMap const &map) const {
    if (data_entry) {
        if (label_entry) {
            auto iter = map.find(label);
            if (iter == map.end()) {
                throw std::runtime_error(
                        "Unresolved label: " + std::to_string(label));
            }
            return iter->second;
        }
        return data;
    }
    return static_cast<uint32_t>(opcode) 
            | (static_cast<uint32_t>(funccode) << 8);
}

Serializer::Serializer()
        : stack({StackEntry(OpCode::Nop)}), 
        counter(2) {}

uint32_t Serializer::get_symbol_id() {
    uint32_t symbol_id = counter;
    counter++;
    return symbol_id;
}

Serializer &Serializer::add_data(uint32_t data) {
    stack.push_back(StackEntry(data));
    return *this;
}

Serializer &Serializer::add_instr(OpCode opcode, FuncCode funccode) {
    stack.push_back(StackEntry(opcode, funccode));
    return *this;
}

uint32_t Serializer::attach_label() {
    uint32_t label = counter;
    stack[stack.size() - 1].set_label(label);
    counter++;
    return label;
}


uint32_t Serializer::attach_label(uint32_t label) {
    stack[stack.size() - 1].set_label(label);
    return label;
}

void Serializer::references_label(uint32_t label) {
    stack[stack.size() - 1].set_label(label);
}

void Serializer::serialize(BaseNode *root) {
    std::vector<std::string> symbol_table; // maps from int_id to identifier
    std::unordered_map<std::string, uint32_t> global_symbol_map;
    uint32_t entry_label;

    root->resolve_symbols_first_pass(*this, global_symbol_map);
    root->resolve_symbols_second_pass(*this, global_symbol_map);
    // Note that 'main' may not be a function name but could be another
    // global scope definition, this is intended behavior.
    auto iter = global_symbol_map.find("main"); // TODO: variable entry point
    if (iter == global_symbol_map.end()) {
        throw std::runtime_error("Entry point 'main' was not defined");
    }
    entry_label = iter->second;

    add_instr(OpCode::Push).add_data(0);
    add_instr(OpCode::Push).add_data().references_label(entry_label);
    add_instr(OpCode::Call);
    add_instr(OpCode::SysCall, FuncCode::Exit);
    root->serialize(*this);
}

void Serializer::assemble(std::ofstream &file) const {
    uint32_t value;
    LabelMap map;
    uint32_t i;
    for (i = 0; i < stack.size(); i++) {
        stack[i].register_label(map, i);
    }
    for (StackEntry const &entry : stack) {
        value = entry.assemble(map);
        file.write(reinterpret_cast<const char*>(&value), 4);
    }
}
