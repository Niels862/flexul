
#ifndef FLEXUL_SYMBOL_HPP
#define FLEXUL_SYMBOL_HPP

#include <cstdint>
#include <string>
#include <unordered_map>

enum class StorageType {
    Invalid,
    // Resolved using labels at assembling with labels placed at serialization
    Absolute,
    // Resolved during serialization
    Relative,
    Alias
};

using SymbolId = uint32_t;

struct SymbolEntry {
    std::string symbol;
    SymbolId id;
    StorageType storage_type;
    uint32_t value;
    uint64_t usages;
};

using SymbolMap = std::unordered_map<std::string, SymbolId>;

SymbolId lookup_symbol(std::string const &symbol, SymbolMap const &global_scope, 
        SymbolMap const &enclosing_scope, SymbolMap const &current_scope);

#endif
