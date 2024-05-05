#include "serializer.hpp"
#include "utils.hpp"
#include "mnemonics.hpp"
#include <iostream>
#include <stdexcept>
#include <unordered_set>
#include <optional>

JobEntry::JobEntry()
        : label(0), node(nullptr), no_serialize(false) {}


JobEntry::JobEntry(uint32_t label, BaseNode *node, bool no_serialize)
        : label(label), node(node), no_serialize(no_serialize) {}

StackEntry::StackEntry()
        : m_type(EntryType::Instruction), m_opcode(OpCode::Nop), 
        m_funccode(FuncCode::Nop), m_data(0), m_has_immediate(0),
        m_references_label(0), m_size(0) {}

StackEntry::StackEntry(EntryType type, OpCode opcode, FuncCode funccode, 
        uint32_t data, bool has_immediate, bool references_label)
        : m_type(type), m_opcode(opcode), m_funccode(funccode), 
        m_data(data), m_has_immediate(has_immediate), 
        m_references_label(references_label), m_size(0) {
    if (type == EntryType::Instruction) {
        m_size = 1 + has_immediate;
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
    switch (m_opcode) {
        case OpCode::Nop:
            return true;
        case OpCode::AddSp:
            return m_has_immediate && m_data == 0;
        case OpCode::Unary:
            switch (m_funccode) {
                case FuncCode::Neg:
                    return m_has_immediate && m_data == 0;
                default:
                    break;
            }
            break;
        case OpCode::Binary:
            switch (m_funccode) {
                case FuncCode::Add:
                case FuncCode::Sub:
                    return m_has_immediate && m_data == 0;
                case FuncCode::Mul:
                case FuncCode::Div:
                    return m_has_immediate && m_data == 1;
                default:
                    break; 
            }
            break;
        default:
            break;
    }
    return false;
}

bool StackEntry::combine(StackEntry const &right, StackEntry &combined) const {
    if (m_type != EntryType::Instruction 
            || right.m_type != EntryType::Instruction) {
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
    if (m_opcode == OpCode::Jump || m_opcode == OpCode::Ret) {
        combined = *this;
        return true; 
        // TODO: After serializing, remove unused labels and check 
        // if jump label; label: can be combined
    }
    if (m_opcode == OpCode::Push && m_has_immediate && !m_references_label
            && right.m_has_immediate 
            && (right.m_opcode == OpCode::BrTrue 
                || right.m_opcode == OpCode::BrFalse)) {
        if ((m_data && right.m_opcode == OpCode::BrTrue) 
                || (!m_data && right.m_opcode == OpCode::BrFalse)) {
            combined = StackEntry::instr(
                    OpCode::Jump, right.m_data, right.m_references_label);
        } else {
            combined = StackEntry::instr(OpCode::Nop);
        }
        return true;
    }
    if (m_opcode == OpCode::Push && m_has_immediate && !right.m_has_immediate 
            && !(right.m_opcode == OpCode::SysCall  // todo better
                && right.m_funccode == FuncCode::GetC)) {
        combined = StackEntry::instr(right.m_opcode, right.m_funccode, 
                m_data, m_references_label);
        return true;
    }
    if (m_opcode == OpCode::LoadAddrRel && m_has_immediate 
            && !m_references_label && right.m_opcode == OpCode::LoadAbs 
            && !right.m_has_immediate) {
        combined = StackEntry::instr(OpCode::LoadRel, m_data, false);
        return true;
    }
    if (m_opcode == OpCode::Push && m_has_immediate && !m_references_label) {
        std::optional<uint32_t> value;
        int32_t left_value = m_data;
        if (right.m_opcode == OpCode::Unary) {
            if (right.m_funccode == FuncCode::Neg) {
                value = -left_value;
            }
        }
        if (right.m_opcode == OpCode::Binary && right.m_has_immediate 
                && !right.m_references_label) {
            int32_t right_value = right.m_data;
            switch (right.m_funccode) {
                case FuncCode::Add:
                    value = left_value + right_value;
                    break;
                case FuncCode::Sub:
                    value = left_value - right_value;
                    break;
                case FuncCode::Mul:
                    value = left_value * right_value;
                    break;
                case FuncCode::Div:
                    value = left_value / right_value;
                    break;
                case FuncCode::Mod:
                    value = left_value % right_value;
                    break;
                default:
                    break;
            }
        }
        if (value.has_value()) {
            combined = StackEntry::instr(OpCode::Push, value.value());
            return true;
        }
    }
    return false;
}

void StackEntry::register_label(LabelMap &map, uint32_t &i) const {
    if (m_type == EntryType::Label) {
        if (map.find(m_data) != map.end()) {
            throw std::runtime_error(
                    "Redefinition of label " + std::to_string(m_data));
        }
        map[m_data] = i;
    }
    i += m_size;
}

void StackEntry::assemble(std::vector<uint32_t> &stack, 
        LabelMap const &map) const {
    uint32_t immediate = m_data;
    if (m_type == EntryType::Invalid) {
        throw std::runtime_error("Invalid entry");
    }
    if (m_references_label) {
        auto iter = map.find(immediate);
        if (iter == map.end()) {
            throw std::runtime_error(
                    "Unresolved label: " + std::to_string(immediate));
        }
        immediate = iter->second;
    }
    if (m_type == EntryType::Instruction) {
        stack.push_back(static_cast<uint32_t>(m_opcode) 
                | (static_cast<uint32_t>(m_funccode) << 8)
                | (m_has_immediate << 7));
        if (m_has_immediate) {
            stack.push_back(immediate);
        }
    }
}

size_t StackEntry::get_size() const {
    return m_size;
}

void StackEntry::disassemble() const {
    if (m_type == EntryType::Label) {
        std::cerr << ".L" << m_data << ":" << std::endl;
    } else if (m_type == EntryType::Instruction) {
        std::cerr << "    " << get_op_name(m_opcode);
        std::string func_name = get_func_name(m_opcode, m_funccode);
        if (!func_name.empty()) {
            std::cerr << "." << func_name;
        }
        if (m_has_immediate) {
            if (m_references_label) {
                std::cerr << " .L" << m_data;
            } else {
                std::cerr << " " << static_cast<int32_t>(m_data);
            }
        }
        std::cerr << std::endl;
    }
}

Serializer::Serializer(SymbolTable &symbol_table)
        : m_symbol_table(symbol_table), m_inline_frames(*this), 
        m_code_jobs(), m_labels(), m_stack() {}

void Serializer::call(SymbolId id, 
        std::vector<std::unique_ptr<ExpressionNode>> const &args) {
    if (id == 0) {
        throw std::runtime_error("No matching call found");
    }
    CallableNode *callable = dynamic_cast<CallableNode *>(
            m_symbol_table.get(id).definition);
    if (callable == nullptr) {
        m_symbol_table.dump();
        throw std::runtime_error("Definition is not callable: " + std::to_string(id));
    }
    add_function_implementation(id);
    callable->serialize_call(*this, args);
}

void Serializer::push_callable_addr(SymbolId id) {
    auto callable = m_symbol_table.callable(id);
    if (callable.size() != 1) {
        throw std::runtime_error(
                "Can only reference single implementation");
    }
    SymbolEntry entry = m_symbol_table.get(callable.front());
    if (entry.storage_type != StorageType::AbsoluteRef) {
        throw std::runtime_error("Can only reference function");
    }
    add_function_implementation(entry.id);
    add_instr(OpCode::Push, entry.id, true);
}

void Serializer::add_instr(OpCode opcode, FuncCode funccode) {
    add_entry(StackEntry::instr(opcode, funccode));
}

void Serializer::add_instr(OpCode opcode, uint32_t data, 
        bool references_label) {
    add_entry(StackEntry::instr(opcode, data, references_label));
}

void Serializer::add_instr(OpCode opcode, FuncCode funccode, 
        uint32_t data, bool references_label) {
    add_entry(StackEntry::instr(opcode, funccode, data, references_label));
}

void Serializer::add_job(uint32_t label, BaseNode *node, bool no_serialize) {
    m_code_jobs.push({label, node, no_serialize});
}

void Serializer::add_function_implementation(SymbolId id) {
    SymbolEntry &symbol = m_symbol_table.m_table[id];
    if (!symbol.implemented) {
        m_code_jobs.push(JobEntry(id, symbol.definition, false));
        symbol.implemented = true;
    }
}

uint32_t Serializer::add_label() {
    return add_label(get_label());
}

uint32_t Serializer::add_label(Label label) {
    add_entry(StackEntry::label(label));
    return label;
}

uint32_t Serializer::get_label() {
    return m_symbol_table.next_id();
}

uint32_t Serializer::get_stack_size() const {
    uint32_t size = 0;
    for (StackEntry const &entry : m_stack) {
        size += entry.get_size();
    }
    return size;
}

void Serializer::serialize() {
    uint32_t global_size = m_symbol_table.container_size();

    SymbolId entry_id = lookup_scope("main", m_symbol_table.global());
    auto callable = m_symbol_table.callable(entry_id);
    if (callable.size() != 1) {
        throw std::runtime_error("Multiple definitions for 'main'");
    }

    add_instr(OpCode::AddSp, global_size);
    call(callable.front(), {});
    add_instr(OpCode::SysCall, FuncCode::Exit);

    while (!m_code_jobs.empty()) {
        JobEntry &job = m_code_jobs.front();
        if (!job.no_serialize) {
            add_label(job.label);
            job.node->serialize(*this);
        }
        m_code_jobs.pop();
    }

    uint32_t position = get_stack_size();
    for (SymbolId const id : m_symbol_table.container()) {
        m_labels[id] = position;
        position += m_symbol_table.get(id).size;
    }
}

std::vector<uint32_t> Serializer::assemble() {
    std::vector<uint32_t> bytecode;
    uint32_t i = 0;
    for (StackEntry const &entry : m_stack) {
        entry.register_label(m_labels, i);
    }
    for (StackEntry const &entry : m_stack) {
        entry.assemble(bytecode, m_labels);
    }
    return bytecode;
}

void Serializer::disassemble() const {
    for (StackEntry const &entry : m_stack) {
        entry.disassemble();
    }
}

SymbolTable &Serializer::symbol_table() {
    return m_symbol_table;
}

InlineFrames &Serializer::inline_frames() {
    return m_inline_frames;
}

void Serializer::add_entry(StackEntry const &entry) {
    m_stack.push_back(entry);
    StackEntry left;
    StackEntry right;
    StackEntry combined;
    while (m_stack.size() > 1) {
        left = m_stack[m_stack.size() - 2];
        right = m_stack[m_stack.size() - 1];
        if (left.combine(right, combined)) {
            m_stack.pop_back();
            m_stack[m_stack.size() - 1] = combined;
        } else {
            return;
        }
    }
}
