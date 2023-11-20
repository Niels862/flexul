
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
    Relative
};

using SymbolId = uint32_t;

struct SymbolEntry {
    SymbolId id;
    StorageType storage_type;
    uint32_t value;
};

using SymbolMap = std::unordered_map<std::string, SymbolId>;

#endif
