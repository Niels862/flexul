#include "serializer.hpp"
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
        map[label] = idx;
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
        {}

Serializer Serializer::add_data(uint32_t data) {
    stack.push_back(StackEntry(data));
    return *this;
}

Serializer Serializer::add_instr(OpCode opcode, FuncCode funccode) {
    stack.push_back(StackEntry(opcode, funccode));
    return *this;
}

Serializer Serializer::with_label(uint32_t label) {
    if (stack.empty()) {
        throw std::runtime_error("Empty program: cannot assign label");
    }
    stack[stack.size() - 1].set_label(label);
    return *this;
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
