
#ifndef FLEXUL_SYMBOL_HPP
#define FLEXUL_SYMBOL_HPP

#include "opcodes.hpp"
#include <cstdint>
#include <string>
#include <unordered_map>
#include <vector>
#include <stack>

enum class StorageType {
    Invalid,
    AbsoluteRef,
    RelativeRef,
    Absolute,
    Relative,
    Intrinsic,
    Callable,
    InlineReference,
    Type
};

using SymbolId = uint32_t;

using SymbolIdList = std::vector<SymbolId>;

class BaseNode;

struct IntrinsicEntry {
    std::string symbol;
    size_t n_args;
    OpCode opcode;
    FuncCode funccode;
};

extern std::vector<IntrinsicEntry> const intrinsics;

struct SymbolEntry {
    std::string symbol;
    SymbolId id;
    StorageType storage_type;
    uint32_t value;
    uint32_t size;
    uint64_t usages;
};

using SymbolMap = std::unordered_map<std::string, SymbolId>;

SymbolId lookup_symbol(std::string const &symbol, SymbolMap const &global_scope, 
        SymbolMap const &enclosing_scope, SymbolMap const &current_scope);

class SymbolTable {
public:
    SymbolTable(SymbolId &counter);

    SymbolId next_id();
    SymbolEntry const &get(SymbolId id) const;
    SymbolId declare(std::string const &symbol, SymbolMap &scope, 
            StorageType storage_type, uint32_t value = 0, 
            uint32_t size = 1);
    void load_predefined(SymbolMap &symbol_map);
    void dump() const;

    void open_container();
    void add_to_container(SymbolId id);
    uint32_t container_size() const;
    void resolve_local_container();
    SymbolIdList const &container() const;
private:
    void add(SymbolEntry const &entry);

    std::vector<SymbolEntry> m_table;
    std::stack<SymbolIdList> m_containers;
    SymbolId &m_counter;
};

#endif
