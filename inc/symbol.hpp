
#ifndef FLEXUL_SYMBOL_HPP
#define FLEXUL_SYMBOL_HPP

#include "opcodes.hpp"
#include <memory>
#include <cstdint>
#include <string>
#include <unordered_map>
#include <vector>
#include <stack>
#include <queue>

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

class TypeNode;

class CallableNode;

class Serializer;

struct IntrinsicEntry {
    std::string symbol;
    size_t n_args;
    OpCode opcode;
    FuncCode funccode;
};

extern std::vector<IntrinsicEntry> const intrinsics;

struct SymbolEntry {
    SymbolEntry(std::string symbol, BaseNode *definition, TypeNode *type, 
            SymbolId id, StorageType storage_type, uint32_t value, 
            uint32_t size);

    void overload_of(SymbolId name_id);

    std::string symbol;
    BaseNode *definition;
    TypeNode *type;
    SymbolId id;
    StorageType storage_type;
    uint32_t value;
    uint32_t size;
    uint64_t usages;
    bool implemented;
    bool overload;
};

using SymbolMap = std::unordered_map<std::string, SymbolId>;

struct ScopeTracker {
    ScopeTracker();
    ScopeTracker(SymbolMap global, SymbolMap enclosing, SymbolMap current);

    SymbolMap global;
    SymbolMap enclosing;
    SymbolMap current;
};

SymbolId lookup_symbol(std::string const &symbol, ScopeTracker const &scopes);

SymbolId lookup_scope(std::string const &symbol, SymbolMap const &scope);

class SymbolTable {
public:
    SymbolTable(std::unique_ptr<BaseNode> &root);

    void resolve();
    void add_job(BaseNode *node);

    SymbolId next_id();
    SymbolEntry const &get(SymbolId id) const;
    SymbolId declare(SymbolMap &scope, std::string const &symbol, 
            BaseNode *definition, TypeNode *type, StorageType storage_type, 
            uint32_t value = 0, uint32_t size = 1);
    SymbolId declare_callable(std::string const &name, SymbolMap &scope, 
            CallableNode *node);

    void load_predefined(SymbolMap &symbol_map);
    void dump() const;

    void open_container();
    void add_to_container(SymbolId id);
    uint32_t container_size() const;
    void resolve_local_container();
    SymbolIdList const &container() const;

    SymbolMap const &global() const;

    std::vector<SymbolEntry>::const_iterator begin() const;
    std::vector<SymbolEntry>::const_iterator end() const;

    SymbolId counter() const;
private:
    void add(SymbolEntry const &entry);

    SymbolMap m_global;

    std::unique_ptr<BaseNode> &m_root;
    std::queue<BaseNode *> m_jobs;
    std::vector<SymbolEntry> m_table;
    std::stack<SymbolIdList> m_containers;
    SymbolId m_counter;

    friend Serializer;
};


#endif
