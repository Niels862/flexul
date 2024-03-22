#include "serializer.hpp"
#include "utils.hpp"
#include "mnemonics.hpp"
#include <iostream>
#include <stdexcept>
#include <unordered_set>
#include <optional>

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

Serializer::Serializer()
        : m_symbol_table(m_counter), m_counter(2), m_stack() {}

SymbolId Serializer::declare_callable(std::string const &name, 
        SymbolMap &scope, CallableNode *node) {
    SymbolMap::const_iterator name_iter = scope.find(name);
    SymbolId name_id;
    if (name_iter == scope.end()) { // New callable
        name_id = m_symbol_table.declare(name, scope, StorageType::Callable);
    } else {
        name_id = name_iter->second;
        if (m_symbol_table.get(name_id).storage_type != StorageType::Callable) {
            throw std::runtime_error("Can only overload other callables");
        }
    }

    CallableMap::iterator iter = m_callable_map.find(name_id);
    if (iter == m_callable_map.end()) {
        CallableEntry callable;
        callable.add_overload(node);
        m_callable_map[name_id] = callable;
    } else {
        iter->second.add_overload(node);
    }
    return m_symbol_table.declare("." + name + "_" + std::to_string(m_counter), 
            scope, StorageType::AbsoluteRef);
}

void Serializer::call(SymbolId id, 
        std::unique_ptr<ExpressionListNode> const &params) {
    CallableMap::const_iterator iter = m_callable_map.find(id);
    if (iter == m_callable_map.end()) {
        throw std::runtime_error("Oh no");
    }
    iter->second.call(*this, params);
}

void Serializer::push_callable_addr(SymbolId id) {
    CallableMap::const_iterator iter = m_callable_map.find(id);
    if (iter == m_callable_map.end()) {
        throw std::runtime_error("Oh no");
    }
    iter->second.push_callable_addr(*this);
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

void Serializer::add_job(uint32_t label, BaseNode *node, bool no_serialize) {
    m_code_jobs.push_back({label, node, no_serialize});
}

uint32_t Serializer::add_label() {
    return add_label(get_label());
}

uint32_t Serializer::add_label(Label label) {
    add_entry(StackEntry::label(label));
    return label;
}

uint32_t Serializer::get_label() {
    Label label = m_counter;
    m_counter++;
    return label;
}

uint32_t Serializer::get_stack_size() const {
    uint32_t size = 0;
    for (StackEntry const &entry : m_stack) {
        size += entry.get_size();
    }
    return size;
}

void Serializer::serialize(std::unique_ptr<BaseNode> &root) {
    ScopeTracker scopes;
    uint32_t i;

    m_symbol_table.load_predefined(scopes.global);
    m_symbol_table.open_container();

    root->resolve_globals(*this, scopes.global);
    for (JobEntry const &job : m_code_jobs) {
        job.node->resolve_locals(*this, scopes);
    }
    
    uint32_t global_size = m_symbol_table.container_size();
    // Note that 'main' may not be a function name but could be another
    // global scope definition, this is intended behavior.
    auto iter = scopes.global.find("main"); // TODO: variable entry point
    if (iter == scopes.global.end()) {
        throw std::runtime_error("Entry point 'main' was not defined");
    }

    add_instr(OpCode::AddSp, global_size);
    call(iter->second, nullptr);
    add_instr(OpCode::SysCall, FuncCode::Exit);

    for (i = 0; i < m_code_jobs.size(); i++) {
        if (!m_code_jobs[i].no_serialize) {
            add_label(m_code_jobs[i].label);
            m_code_jobs[i].node->serialize(*this);
        }
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
