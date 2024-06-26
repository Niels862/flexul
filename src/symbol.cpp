#include "symbol.hpp"
#include "utils.hpp"
#include "tree.hpp"
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

SymbolEntry::SymbolEntry(std::string symbol, BaseNode *definition, 
            TypeNode *type, SymbolId id, StorageType storage_type, 
            uint32_t value, uint32_t size)
            : symbol(symbol), definition(definition), type(type), id(id),
            storage_type(storage_type), value(value), size(size),
            usages(0), implemented(false), overload(false) {}

void SymbolEntry::overload_of(SymbolId name_id) {
    overload = true;
    value = name_id;
}

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

SymbolId lookup_scope(std::string const &symbol, SymbolMap const &scope) {
    SymbolMap::const_iterator iter = scope.find(symbol);
    if (iter == scope.end()) {
        throw std::runtime_error(symbol + " not defined in scope");
    }
    return iter->second;
}

SymbolTable::SymbolTable(std::unique_ptr<BaseNode> &root)
        : m_root(root), m_jobs(), m_table({
            SymbolEntry(
                "<null>", nullptr, nullptr, 0, StorageType::Invalid, 0, 0),
            SymbolEntry(
                "<entry>", nullptr, nullptr, 1, StorageType::AbsoluteRef, 0, 0)
        }), m_counter(2) {}

void SymbolTable::resolve() {
    ScopeTracker scopes;

    load_predefined(scopes.global);
    open_container();

    m_root->resolve_globals(*this, scopes.global);
    
    for (SymbolEntry const &entry : m_table) {
        if (entry.overload) {
            m_callables[entry.value].push_back(entry.id);
        }
    }

    while (!m_jobs.empty()) {
        BaseNode *job = m_jobs.front();
        job->resolve_locals(*this, scopes);
        job->resolve_types(*this);
        m_jobs.pop();
    }

    m_global = std::move(scopes.global);
}

void SymbolTable::add_job(BaseNode *node) {
    m_jobs.push(node);
}

SymbolId SymbolTable::next_id() {
    SymbolId id = m_counter;
    m_counter++;
    return id;
}

SymbolEntry const &SymbolTable::get(SymbolId id) const {
    return m_table[id];
}

SymbolId SymbolTable::declare(SymbolMap &scope, std::string const &symbol, 
        BaseNode *definition, TypeNode *type, StorageType storage_type, 
        uint32_t value, uint32_t size) {
    SymbolMap::const_iterator iter = scope.find(symbol);
    if (iter != scope.end()) {
        print_map(scope);
        dump();
        throw std::runtime_error("Redeclared symbol: " + symbol);
    }
    SymbolId id = next_id();
    scope[symbol] = id;
    add(SymbolEntry(symbol, definition, type, id, storage_type, value, size));
    return id;
}

SymbolId SymbolTable::declare_callable(std::string const &name, 
        SymbolMap &scope, CallableNode *node) {
    SymbolMap::const_iterator name_iter = scope.find(name);
    SymbolId name_id;
    if (name_iter == scope.end()) { // New callable
        name_id = declare(scope, name, nullptr, nullptr, 
                StorageType::Callable);
    } else {
        name_id = name_iter->second;
        if (get(name_id).storage_type != StorageType::Callable) {
            throw std::runtime_error("Can only overload other callables");
        }
    }

    SymbolId definition_id = declare(scope, 
            "." + name + "_" + std::to_string(counter()), 
            node, node->signature().type.get(), StorageType::AbsoluteRef);

    m_table[definition_id].overload_of(name_id);
    
    return definition_id;
}

void SymbolTable::load_predefined(SymbolMap &symbol_map) {
    size_t i;
    for (i = 0; i < intrinsics.size(); i++) {
        IntrinsicEntry intrinsic = intrinsics[i];
        declare(symbol_map, intrinsic.symbol, nullptr, &Any,
                StorageType::Intrinsic, i);
    }
}

void SymbolTable::dump() const {
    uint32_t i;
    for (i = 0; i < m_table.size(); i++) {
        SymbolEntry entry = m_table[i];
        std::cout << std::setw(6) << i << " " << entry.symbol << " symtype=" 
                << static_cast<int>(entry.storage_type);
        if (entry.storage_type != StorageType::Callable 
                && entry.storage_type != StorageType::Intrinsic 
                && entry.storage_type != StorageType::Type) {
            if (entry.type != nullptr) {
                std::cout << " type=" << to_string(entry.type);
            } else {
                std::cout << " type=Any";
            }
        }
        if (entry.overload) {
            std::cout << " value=" << entry.value;
        }
        std::cout << std::endl;
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

SymbolMap const &SymbolTable::global() const {
    return m_global;
}

std::vector<SymbolId> const &SymbolTable::callable(SymbolId id) const {
    auto iter = m_callables.find(id);
    if (iter == m_callables.end()) {
        throw std::runtime_error(m_table[id].symbol + " " 
                + std::to_string(id) + " not in map");
    }
    return iter->second;
}

std::vector<SymbolEntry>::const_iterator SymbolTable::begin() const {
    return m_table.begin();
}

std::vector<SymbolEntry>::const_iterator SymbolTable::end() const {
    return m_table.end();
}

SymbolId SymbolTable::counter() const {
    return m_counter;
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
