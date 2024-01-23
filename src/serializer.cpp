#include "serializer.hpp"
#include "utils.hpp"
#include "mnemonics.hpp"
#include <iostream>
#include <stdexcept>
#include <unordered_set>

std::vector<IntrinsicEntry> const intrinsics = {
    {"__exit__", 1, OpCode::SysCall, FuncCode::Exit},
    {"__putc__", 1, OpCode::SysCall, FuncCode::PutC},
    {"__getc__", 0, OpCode::SysCall, FuncCode::GetC},
    {"__ineg__", 1, OpCode::Unary, FuncCode::Neg},
    {"__iadd__", 2, OpCode::Binary, FuncCode::Add},
    {"__isub__", 2, OpCode::Binary, FuncCode::Sub},
    {"__idiv__", 2, OpCode::Binary, FuncCode::Div},
    {"__imul__", 2, OpCode::Binary, FuncCode::Mul},
    {"__imod__", 2, OpCode::Binary, FuncCode::Mod},
    {"__ieq__", 2, OpCode::Binary, FuncCode::Equals},
    {"__ineq__", 2, OpCode::Binary, FuncCode::NotEquals},
    {"__ilt__", 2, OpCode::Binary, FuncCode::LessThan},
    {"__ile__", 2, OpCode::Binary, FuncCode::LessEquals},
};

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
    if (opcode == OpCode::Push && has_immediate && !right.has_immediate 
            && !(right.opcode == OpCode::SysCall  // todo better
                && right.funccode == FuncCode::GetC)) {
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

size_t StackEntry::get_size() const {
    return size;
}

void StackEntry::disassemble() const {
    if (type == EntryType::Label) {
        std::cerr << ".L" << data << ":" << std::endl;
    } else if (type == EntryType::Instruction) {
        std::cerr << "    " << get_op_name(opcode);
        std::string func_name = get_func_name(opcode, funccode);
        if (!func_name.empty()) {
            std::cerr << "." << func_name;
        }
        if (has_immediate) {
            if (references_label) {
                std::cerr << " .L" << data;
            } else {
                std::cerr << " " << data;
            }
        }
        std::cerr << std::endl;
    }
}

Serializer::Serializer()
        : symbol_table({
            {"<null>", 0, StorageType::Invalid, 0, 0, 0}, 
            {"<entry>", 1, StorageType::Label, 0, 0, 0}
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
        SymbolMap &scope, StorageType storage_type, uint32_t value, 
        uint32_t size) {
    SymbolMap::const_iterator iter = scope.find(symbol);
    if (iter != scope.end()) {
        throw std::runtime_error("Redeclared symbol: " + symbol);
    }
    SymbolId id = get_symbol_id();
    scope[symbol] = id;
    register_symbol({symbol, id, storage_type, value, size, 0});
    return id;
}

SymbolId Serializer::declare_callable(std::string const &name, 
        SymbolMap &scope, CallableNode *node) {
    SymbolMap::const_iterator name_iter = scope.find(name);
    SymbolId name_id;
    if (name_iter == scope.end()) { // New callable
        name_id = declare_symbol(name, scope, StorageType::Callable);
    } else {
        name_id = name_iter->second;
        if (symbol_table[name_id].storage_type != StorageType::Callable) {
            throw std::runtime_error("Can only overload other callables");
        }
    }

    CallableMap::iterator iter = callables.find(name_id);
    if (iter == callables.end()) {
        CallableEntry callable;
        callable.add_overload(node);
        callables[name_id] = callable;
    } else {
        iter->second.add_overload(node);
    }
    return declare_symbol("." + name + "_" + std::to_string(counter), 
            scope, StorageType::Label);
}

void Serializer::open_container() {
    containers.push(std::vector<SymbolId>());
}

void Serializer::add_to_container(SymbolId id) {
    containers.top().push_back(id);
}

uint32_t Serializer::get_container_size() const {
    uint32_t size = 0;
    for (SymbolId const id : containers.top()) {
        size += symbol_table[id].size;
    }
    return size;
}

void Serializer::resolve_local_container() {
    uint32_t position = 0;
    for (SymbolId const id : containers.top()) {
        symbol_table[id].value = position;
        position += symbol_table[id].size;
    }
    containers.pop();
}

void Serializer::open_inline_call(BaseNode *params) {
    inline_params.push(params);
}

void Serializer::use_inline_param(uint32_t index) {
    inline_params.top()->get(index)->serialize(*this);
}

void Serializer::close_inline_call() {
    inline_params.pop();
}

void Serializer::call(SymbolId id, BaseNode *params) {
    CallableMap::const_iterator iter = callables.find(id);
    if (iter == callables.end()) {
        throw std::runtime_error("Oh no");
    }
    iter->second.call(*this, params);
}

void Serializer::push_callable_addr(SymbolId id) {
    CallableMap::const_iterator iter = callables.find(id);
    if (iter == callables.end()) {
        throw std::runtime_error("Oh no");
    }
    iter->second.push_callable_addr(*this);
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

void Serializer::add_job(uint32_t label, BaseNode *node, bool no_serialize) {
    code_jobs.push_back({label, node, no_serialize});
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

uint32_t Serializer::get_stack_size() const {
    uint32_t size = 0;
    for (StackEntry const &entry : stack) {
        size += entry.get_size();
    }
    return size;
}

void Serializer::load_predefined(SymbolMap &symbol_map) {
    size_t i;
    for (i = 0; i < intrinsics.size(); i++) {
        IntrinsicEntry intrinsic = intrinsics[i];
        declare_symbol(intrinsic.symbol, symbol_map, StorageType::Intrinsic, i);
    }
}

void Serializer::serialize(BaseNode *root) {
    SymbolMap global_scope;
    SymbolMap enclosing_scope;
    SymbolMap current_scope;
    uint32_t i;

    load_predefined(global_scope);
    open_container();

    root->resolve_symbols_first_pass(*this, global_scope);
    for (JobEntry const &job : code_jobs) {
        job.node->resolve_symbols_second_pass(*this, global_scope, 
                enclosing_scope, current_scope);
    }
    
    uint32_t global_size = get_container_size();
    // Note that 'main' may not be a function name but could be another
    // global scope definition, this is intended behavior.
    auto iter = global_scope.find("main"); // TODO: variable entry point
    if (iter == global_scope.end()) {
        throw std::runtime_error("Entry point 'main' was not defined");
    }

    add_instr(OpCode::AddSp, global_size);
    call(iter->second, nullptr);
    add_instr(OpCode::SysCall, FuncCode::Exit);

    for (i = 0; i < code_jobs.size(); i++) {
        if (!code_jobs[i].no_serialize) {
            add_label(code_jobs[i].label);
            code_jobs[i].node->serialize(*this);
        }
    }

    uint32_t position = get_stack_size();
    for (SymbolId const id : containers.top()) {
        labels[id] = position;
        position += symbol_table[id].size;
    }

    dump_symbol_table();
}

std::vector<uint32_t> Serializer::assemble() {
    std::vector<uint32_t> bytecode;
    uint32_t i = 0;
    for (StackEntry const &entry : stack) {
        entry.register_label(labels, i);
    }
    for (StackEntry const &entry : stack) {
        entry.assemble(bytecode, labels);
    }
    return bytecode;
}

void Serializer::disassemble() const {
    for (StackEntry const &entry : stack) {
        entry.disassemble();
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
