#include "symbol.hpp"
#include "utils.hpp"
#include <stdexcept>

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

ScopeTracker::ScopeTracker()
        : global(), enclosing(), current() {}

ScopeTracker::ScopeTracker(SymbolMap global, SymbolMap enclosing, 
        SymbolMap current)
        : global(global), enclosing(enclosing), current(current) {}

SymbolId lookup_symbol(std::string const &symbol, ScopeTracker const &scopes) {
    SymbolMap::const_iterator iter;

    iter = scopes.current.find(symbol);
    if (iter != scopes.current.end()) {
        return iter->second;
    }
    iter = scopes.enclosing.find(symbol);
    if (iter != scopes.enclosing.end()) {
        return iter->second;
    }
    iter = scopes.global.find(symbol);
    if (iter != scopes.global.end()) {
        return iter->second;
    }
    throw std::runtime_error("Undeclared symbol: " + symbol);
}

SymbolTable::SymbolTable(SymbolId &counter)
        : m_table({
            {"<null>", 0, StorageType::Invalid, 0, 0, 0}, 
            {"<entry>", 1, StorageType::AbsoluteRef, 0, 0, 0}
        }), m_counter(counter) {}

SymbolId SymbolTable::next_id() {
    SymbolId id = m_counter;
    m_counter++;
    return id;
}

SymbolEntry const &SymbolTable::get(SymbolId id) const {
    return m_table[id];
}

SymbolId SymbolTable::declare(std::string const &symbol, 
        SymbolMap &scope, StorageType storage_type, uint32_t value, 
        uint32_t size) {
    SymbolMap::const_iterator iter = scope.find(symbol);
    if (iter != scope.end()) {
        throw std::runtime_error("Redeclared symbol: " + symbol);
    }
    SymbolId id = next_id();
    scope[symbol] = id;
    add({symbol, id, storage_type, value, size, 0});
    return id;
}

void SymbolTable::load_predefined(SymbolMap &symbol_map) {
    size_t i;
    for (i = 0; i < intrinsics.size(); i++) {
        IntrinsicEntry intrinsic = intrinsics[i];
        declare(intrinsic.symbol, symbol_map, 
                StorageType::Intrinsic, i);
    }
}

void SymbolTable::dump() const {
    uint32_t i;
    for (i = 0; i < m_table.size(); i++) {
        SymbolEntry entry = m_table[i];
        std::cerr << std::setw(6) << i << ": " 
                << entry.symbol << " of type " 
                << static_cast<int>(entry.storage_type)
                << " with value " << static_cast<int32_t>(entry.value) 
                << " (" << entry.usages << " usages)" << std::endl;
    }
}

void SymbolTable::open_container() {
    m_containers.push(std::vector<SymbolId>());
}

void SymbolTable::add_to_container(SymbolId id) {
    m_containers.top().push_back(id);
}

uint32_t SymbolTable::container_size() const {
    uint32_t size = 0;
    for (SymbolId const id : m_containers.top()) {
        size += m_table[id].size;
    }
    return size;
}

void SymbolTable::resolve_local_container() {
    uint32_t position = 0;
    for (SymbolId const id : m_containers.top()) {
        m_table[id].value = position;
        position += m_table[id].size;
    }
    m_containers.pop();
}

SymbolIdList const &SymbolTable::container() const {
    return m_containers.top();
}

void SymbolTable::add(SymbolEntry const &entry) {
    if (entry.id != m_table.size()) {
        throw std::runtime_error(
                "Registered symbol ID does not match expected value: got "
                + std::to_string(entry.id) + ", expected "
                + std::to_string(m_table.size()));
    }
    m_table.push_back(entry);
}
